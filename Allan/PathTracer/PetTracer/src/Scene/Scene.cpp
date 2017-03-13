#include "Scene.h"
#include "../BVH/BVH.h"
#include "../BVH/PlainBVHTranslator.h"

#include "tiny_obj_loader.h"

#include <fstream>

namespace PetTracer
{
	Scene::Scene(CLWContext const& context )
		: mOpenCLContext(context), 
		  mTrianglesIndexes( context, ReadOnly ),
		  mVerticesPosition( context, ReadOnly ),
		  mVerticesNormal( context, ReadOnly ),
		  mVerticesTexCoord( context, ReadOnly ),
		  mBVHNodes( context, ReadOnly )
	{


	}

	Scene::Scene( CLWContext const& context, const char* filePath, bool uploadscene )
		: mOpenCLContext( context ),
		  mTrianglesIndexes( context, ReadOnly ),
		  mVerticesPosition( context, ReadOnly ),
		  mVerticesNormal( context, ReadOnly ),
		  mVerticesTexCoord( context, ReadOnly ),
		  mBVHNodes( context, ReadOnly )
	{
		OpenFile( filePath, uploadscene );
	}

	Scene::~Scene()
	{
	}

	bool Scene::OpenFile( const char* filePath, bool UploadToGPU )
	{
		mScenePath = filePath;

		// Load and preprocess the obj file
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err = "";

		// Get the file folder path
		std::string fPath( filePath );
		size_t lastSlash = fPath.find_last_of( '/' );
		lastSlash = ( lastSlash == -1 ) ? fPath.find_last_of( '\\' ) : lastSlash;
		std::string folderPath = fPath.substr( 0, lastSlash );
		
		mTriangleCount = 0;
		mVertexCount = 0;

		// Try to open the file
		err = tinyobj::LoadObj( shapes, materials, filePath, folderPath.c_str() );
		if ( err == "" )
		{

			// Calculate the number of triangles and vertex
			// for earch shape in the scene
			for ( tinyobj::shape_t& shape : shapes )
			{
				tinyobj::mesh_t& mesh = shape.mesh;

				uint32 vtxCount = (uint32) mesh.positions.size();
				uint32 idxCount = (uint32) mesh.indices.size();

				mTriangleCount	+= ( idxCount / 3 );
				mVertexCount	+= ( vtxCount / 3 );

			}

			// Allocate alf the memory nescessary
			mTrianglesIndexes.Alloc( mTriangleCount );
			mVerticesPosition.Alloc( mVertexCount );
			mVerticesTexCoord.Alloc( mVertexCount );
			mVerticesNormal.Alloc( mVertexCount );


			{
				int4*	triIdx = mTrianglesIndexes.GetPointer();
				float4* vtxPos = mVerticesPosition.GetPointer();
				float4* vtxTex = mVerticesTexCoord.GetPointer();
				float4* vtxNor = mVerticesNormal.GetPointer();
				uint32  vtxOff = 0;
				uint32  idxOff = 0;

				// Fill the buffer with scene data
				int32 shapeID = 2;
				for ( tinyobj::shape_t& shape : shapes )
				{
					tinyobj::mesh_t& mesh = shape.mesh;
					uint32 vtxCount = (uint32) mesh.positions.size() / 3;
					uint32 idxCount = (uint32) mesh.indices.size() / 3;
					bool hasTexCor = mesh.texcoords.size() != 0;
					bool hasNormal = mesh.normals.size() != 0;

					for ( uint32 i = 0; i < vtxCount; i++ )
					{
						vtxPos[i + vtxOff] = float4( mesh.positions[i * 3], mesh.positions[i * 3 + 1], mesh.positions[i * 3 + 2] );
						//if( hasTexCor )	vtxTex[i + vtxOff] = float4( mesh.texcoords[i * 3], mesh.texcoords[i * 3 + 1], mesh.texcoords[i * 3 + 2] );
						//if( hasNormal ) vtxNor[i + vtxOff] = float4( mesh.normals  [i * 3], mesh.normals  [i * 3 + 1], mesh.normals  [i * 3 + 2] );
					}

					for ( uint32 i = 0; i < idxCount; i++ )
					{
						triIdx[i + idxOff] = int4( mesh.indices[i * 3] + vtxOff, mesh.indices[i * 3 + 1] + vtxOff, mesh.indices[i * 3 + 2] + vtxOff, shapeID );
					}

					vtxOff += vtxCount;
					idxOff += idxCount;
					//shapeID;
				}
			}

			if ( UploadToGPU )
				UploadScene();

			return true;
		}
		else
		{
			std::cout << "Read file error: " << err << std::endl;
			return false;
		}
	}

	void Scene::UploadScene( bool block )
	{
		mTrianglesIndexes.UploadToGPU( block );
		mVerticesPosition.UploadToGPU( block );
		mVerticesTexCoord.UploadToGPU( block );
		mVerticesNormal.UploadToGPU( block );

		if ( mBVHNodes.GetSize() != 0 )
			mBVHNodes.UploadToGPU( block );
	}

	void Scene::BuildBVH( BuildParams const& params )
	{
		// Search for bvh in file
		std::string path;
		if ( BVHExistis( path ) )
		{
			LoadBVHFromFile( path.c_str() );
			UploadScene( true );
		}
		else
		{
			// if its not available, contruct bvh
			BVH sceneBVH( this, params );
			UpdateBVH( sceneBVH );
			UploadScene( true );
		}

	}

	void Scene::UpdateBVH( BVH const& bvh )
	{
		PlainBVHTranslator translator;
		translator.Process( bvh );

		// Save the BVH to file
		std::string fileName = BVHFileName();
		std::ofstream file;
		file.open( fileName.c_str(), std::ios::out | std::ios::trunc | std::ios::binary );
		file.write( ( char* ) translator.mNodes.data(), sizeof( AABB ) * translator.mNodes.size() );
		file.close();

		/*std::vector<int32> const& nTriIdx = bvh.TriangleIndices();

		// Update scene triangle indexes with the new one created by the bvh (may contain duplicates)
		int4* temp = new int4[mTrianglesIndexes.GetSize()];
		memcpy( temp, mTrianglesIndexes.GetPointer(), mTrianglesIndexes.GetSizeInBytes() );

		mTrianglesIndexes.Alloc( nTriIdx.size() );
		int4* triangleVertices = mTrianglesIndexes.GetPointer();

		for ( int32 i = 0; i < (int32)nTriIdx.size(); i++ )
		{
			triangleVertices[i] = temp[nTriIdx[i]];
		}

		// Free temp memory
		delete[ ] temp;*/

		// Allocate memory and copy the bvh nodes to the buffer
		mBVHNodes.Alloc( translator.mNodeCount );
		memcpy( mBVHNodes.GetPointer(), translator.mNodes.data(), mBVHNodes.GetSizeInBytes() );
	}

	void Scene::LoadBVHFromFile( const char * filename )
	{
		std::ifstream file;
		file.open(filename, std::ios::ate | std::ios::binary );
		uint64 size = file.tellg();
		file.seekg( 0, std::ios::beg );
		mBVHNodes.Alloc( size / sizeof( AABB ) );
		
		int64 bufferSize = 512, bytesLeft = size;
		int64 num = ( size + bufferSize - 1 ) / bufferSize;
		for ( int64 i = 0; i < num; i++ )
		{
			int64 readSize = min( bytesLeft, bufferSize );
			bytesLeft -= readSize;
			file.read( ( char* ) mBVHNodes.GetPointer() + (i * bufferSize), readSize );
			printf( "Scene: Reading from file: progress %.0f%%\r", (float)i/(float)num * 100.0f );
		}

	}

	std::string Scene::BVHFileName()
	{
		return mScenePath.substr( 0, mScenePath.find_last_of( '.' ) ) + ".bvh"; mScenePath.substr( 0, mScenePath.find_last_of( '.' ) ) + ".bvh";
	}

	bool Scene::BVHExistis( std::string & path )
	{
		path = BVHFileName();

		std::ifstream f( path.c_str() );
		return f.good();
	}



}