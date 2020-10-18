#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <assimp/cimport.h>        // Plain-C interface
#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags

#define  TRM_NDC_ZERO_TO_ONE
#include "tr_math.h"
#include "mestack.h"

#define WEIGHTS_PER_VERTEX   4
#define MAX_BONE_NAME_LENGTH 64

//#pragma pack(push, 1)
typedef struct Vertex
{
    vec3     pos;
    vec3     normal;
    vec3     color;
    uint32_t bone_indices[WEIGHTS_PER_VERTEX];
    float    bone_weights[WEIGHTS_PER_VERTEX];
} Vertex;
//#pragma pack(pop)

typedef struct Bone
{
    char name[MAX_BONE_NAME_LENGTH];
    mat4 offset_matrix;
} Bone;

typedef struct Node
{
    uint32_t bone_index;
    int      parent_index;
    char     name[MAX_BONE_NAME_LENGTH];
} Node;

vec3 aiVector3D_to_vec3(struct aiVector3D vert)
{
    vec3 result;
    result.x = vert.x;
    result.y = vert.y;
    result.z = vert.z;
    return result;
}

mat4 aiMatrix4x4_to_mat4(struct aiMatrix4x4 m)
{
    mat4 result = { 0 };
    result.d[0][0] = m.a1;
    result.d[1][0] = m.a2;
    result.d[2][0] = m.a3;
    result.d[3][0] = m.a4;

    result.d[0][1] = m.b1;
    result.d[1][1] = m.b2;
    result.d[2][1] = m.b3;
    result.d[3][1] = m.b4;

    result.d[0][2] = m.c1;
    result.d[1][2] = m.c2;
    result.d[2][2] = m.c3;
    result.d[3][2] = m.c4;

    result.d[0][3] = m.d1;
    result.d[1][3] = m.d2;
    result.d[2][3] = m.d3;
    result.d[3][3] = m.d4;

    return result;
}

void build_skeleton(struct aiNode * ai_node, Node * skeleton_nodes, int * current_node_index, int * current_depth,
		    Bone * bones, uint32_t num_bones)
{
    MeStack children = mes_create(sizeof(struct aiNode *), num_bones);
    for (uint32_t i = 0; i < ai_node ->mNumChildren; ++i) {
	mes_push( children, &(ai_node->mChildren[i]) );	    
    }
    int offset = *current_node_index;
    while ( !mes_empty(children) ) {
	struct aiNode * ai_node;
	mes_pop(children, &ai_node);
	/* Search the bones array for matching name of current node */
	uint32_t bone_index = 0;
	for ( ; bone_index < num_bones; ++bone_index) {
	    if ( !strcmp(ai_node->mName.data, bones[bone_index].name) ) {
		break;
	    }
	}
	if (bone_index < num_bones) {
	    Node node = { .bone_index = 0, .parent_index = -1 };
	    node.bone_index = bone_index;
	    if ( *current_depth == 0 ) {
		node.parent_index = -1;
	    }
	    else {
		node.parent_index = offset - 1;
	    }
	    strcpy( node.name, bones[bone_index].name );
	    skeleton_nodes[ (*current_node_index)++ ] = node;
	}
	(*current_depth)++;
	build_skeleton( ai_node, skeleton_nodes, current_node_index, current_depth, bones, num_bones );
    }
    (*current_depth)--;
    mes_destroy(&children);
}

int main(int argc, char ** argv)
{
    assert(argc > 1);
    char * filename = argv[1];
    struct aiScene const * scene = aiImportFile(filename,
						aiProcess_Triangulate |
						aiProcess_JoinIdenticalVertices);
    //aiProcess_MakeLeftHanded |
    //aiProcess_FlipWindingOrder);
    if (NULL == scene) {
	printf( "%s\n", aiGetErrorString() );
	return -1;
    }

    struct aiMesh * mesh = scene->mMeshes[0];

    uint32_t num_verts = (uint32_t)mesh->mNumVertices;
    uint32_t num_faces = (uint32_t)mesh->mNumFaces;
    uint32_t num_bones = (uint32_t)mesh->mNumBones;
    uint32_t verts_per_face = 3;
    uint32_t num_indices = num_faces * verts_per_face;

    /* Print Debug Info */
    for (uint32_t i = 0; i < num_bones; ++i) {
	struct aiBone * bone = mesh->mBones[i];
	struct aiString bone_name = bone->mName;
	struct aiMatrix4x4 matrix = bone->mOffsetMatrix;
	printf("Bone %i:\n", i);
	printf("    Name: %s\n", bone_name.data);
	printf("    Offset Matrix:\n");
	printf("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
	       matrix.a1, matrix.a2, matrix.a3, matrix.a4,
	       matrix.b1, matrix.b2, matrix.b3, matrix.b4,
	       matrix.c1, matrix.c2, matrix.c3, matrix.c4,
	       matrix.d1, matrix.d2, matrix.d3, matrix.d4	       
	    );
    }

    /* Position, Normal and Colors */
    Vertex * verts = (Vertex*)malloc(num_verts * sizeof(Vertex));
    Vertex * current_vert = verts;
    for (uint32_t i = 0; i < num_verts; ++i) {
	current_vert[i].pos = aiVector3D_to_vec3( mesh->mVertices[i] );
	current_vert[i].normal = aiVector3D_to_vec3( mesh->mNormals[i] );
	current_vert[i].color.x = 0.5*current_vert[i].normal.x+0.5;
	current_vert[i].color.y = 0.5*current_vert[i].normal.y+0.5;
	current_vert[i].color.z = 0.5*current_vert[i].normal.z+0.5;
	memset(current_vert[i].bone_indices, 0, WEIGHTS_PER_VERTEX*sizeof(uint32_t));
	memset(current_vert[i].bone_weights, 0, WEIGHTS_PER_VERTEX*sizeof(float));
    } 

    /* Weights and Bone Indices */
    uint32_t num_weights = num_verts * WEIGHTS_PER_VERTEX;
    uint32_t * bone_indices = (uint32_t*)malloc(num_weights*sizeof(uint32_t));
    memset(bone_indices, 0, num_weights*sizeof(uint32_t));
    float * bone_weights = (float*)malloc(num_weights*sizeof(float));
    memset(bone_weights, 0, num_weights*sizeof(float));
    Bone * bones = (Bone*)malloc(num_bones*sizeof(Bone));
    for (uint32_t i = 0; i < num_bones; ++i) {
	struct aiBone * bone = mesh->mBones[i];
	struct aiMatrix4x4 offset_matrix = bone->mOffsetMatrix;
	struct aiString bone_name = bone->mName;
	bones[i].offset_matrix = aiMatrix4x4_to_mat4(offset_matrix);
	strcpy(bones[i].name, bone_name.data);
	uint32_t num_weights = bone->mNumWeights;
	for (uint32_t j = 0; j < num_weights; ++j) {
	    struct aiVertexWeight ai_weight = bone->mWeights[j];	    
	    uint32_t vertex_id = (uint32_t)ai_weight.mVertexId;
	    uint32_t vertex_start = vertex_id * WEIGHTS_PER_VERTEX;
	    for (uint32_t k = 0; k < WEIGHTS_PER_VERTEX; ++k) {
		#if 0
		if (bone_weights[vertex_start + k] <= 0.0) {
		    bone_indices[vertex_start + k] = i;
		    bone_weights[vertex_start + k] = ai_weight.mWeight;
		    verts[vertex_id].bone_indices[k] = i;
		    verts[vertex_id].bone_weights[k] = ai_weight.mWeight;
		    break;
		}
		#endif
		if (verts[vertex_id].bone_weights[k] <= 0.0) {
		    verts[vertex_id].bone_indices[k] = i;
		    verts[vertex_id].bone_weights[k] = ai_weight.mWeight;
		    break;
		}
	    }
	}
    }
    free(bone_indices);
    free(bone_weights);

    /* Skeleton */
    /* Find the "Armature" Node, as it contains the bone-hierarchy */
    struct aiNode * ai_root = scene->mRootNode;
    struct aiNode * ai_current = NULL;
    for (uint32_t i = 0; i < ai_root->mNumChildren; ++i) {
	ai_current = ai_root->mChildren[i];
	if ( !strcmp("Armature", ai_current->mName.data) ) {
	    break;
	}
    }
    assert(ai_current != NULL);
    Node * skeleton_nodes = (Node*)malloc(num_bones*sizeof(Node));
    int current_node_index = 0;
    int current_depth = 0;
    build_skeleton( ai_current, skeleton_nodes, &current_node_index, &current_depth, bones, num_bones );
    
    /* Index Data for vertices */
    uint16_t * indices = (uint16_t*)malloc(num_indices * sizeof(uint16_t));
    uint16_t * current_index = indices;
    for (uint32_t i = 0; i < num_faces; ++i) {
	current_index[0] = mesh->mFaces[i].mIndices[0];
	current_index[1] = mesh->mFaces[i].mIndices[1];
	current_index[2] = mesh->mFaces[i].mIndices[2];		
	current_index += 3;
    }

//    uint32_t dummy1[] = { 0xFF, 0xFF, 0xFF };
//    uint32_t dummy2[] = { 0xFF00, 0xFF00, 0xFF00, 0xFF00 };
    /* Write it out to disk */
    FILE * filep = fopen( "modeldata.md", "wb" );
    uint32_t magic_number = 0xAABBCCDD;    
    fwrite( &magic_number, sizeof(uint32_t), 1, filep );
    fwrite( &num_verts, sizeof(uint32_t), 1, filep );
    fwrite( &num_indices, sizeof(uint32_t), 1, filep );
    fwrite( &num_bones, sizeof(uint32_t), 1, filep );
    fwrite( verts, sizeof(*verts), num_verts, filep );
    fwrite( indices, sizeof(*indices), num_indices, filep );
    fwrite( bones, sizeof(*bones), num_bones, filep );
    fwrite( skeleton_nodes, sizeof(*skeleton_nodes), num_bones, filep );
    fclose( filep );
    
    aiReleaseImport(scene);
    
    return 0;
}
