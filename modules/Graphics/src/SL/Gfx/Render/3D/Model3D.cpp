#include "Model3D.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <glm/geometric.hpp>

namespace sl {

	void Model3D::AddToScene( SceneNode3D& node ) const
	{
		for ( const auto& part : parts )
			node.instances.push_back( { part.mesh, part.material } );
	}

	namespace {

		struct VertexKey
		{
			int position = 0;
			int texCoord = 0;
			int normal = 0;
			bool operator==( const VertexKey& other ) const = default;
		};

		struct VertexKeyHash
		{
			size_t operator()( const VertexKey& key ) const
			{
				size_t h = std::hash< int >{}( key.position );
				h ^= std::hash< int >{}( key.texCoord ) + 0x9e3779b9 + ( h << 6 ) + ( h >> 2 );
				h ^= std::hash< int >{}( key.normal ) + 0x9e3779b9 + ( h << 6 ) + ( h >> 2 );
				return h;
			}
		};

		int ResolveIndex( int index, size_t size )
		{
			const int resolved = index > 0 ? index - 1 : static_cast< int >( size ) + index;
			if ( index == 0 || resolved < 0 || resolved >= static_cast< int >( size ) )
				throw std::runtime_error( "OBJ index out of bounds" );
			return resolved;
		}

		VertexKey ParseKey( const std::string& text )
		{
			VertexKey key;
			const auto a = text.find( '/' );
			if ( a == std::string::npos ) { key.position = std::stoi( text ); return key; }
			key.position = std::stoi( text.substr( 0, a ) );
			const auto b = text.find( '/', a + 1 );
			if ( b == std::string::npos )
			{
				if ( a + 1 < text.size() ) key.texCoord = std::stoi( text.substr( a + 1 ) );
			}
			else
			{
				if ( b > a + 1 ) key.texCoord = std::stoi( text.substr( a + 1, b - a - 1 ) );
				if ( b + 1 < text.size() ) key.normal = std::stoi( text.substr( b + 1 ) );
			}
			return key;
		}

		using MaterialMap = std::unordered_map< std::string, Material3D >;

		void ReadMaterials( const std::filesystem::path& file, MaterialMap& materials )
		{
			std::ifstream stream( file );
			if ( !stream ) return;
			Material3D* current = nullptr;
			std::string line;
			while ( std::getline( stream, line ) )
			{
				std::istringstream input( line );
				std::string type;
				input >> type;
				if ( type == "newmtl" )
				{
					std::string name;
					input >> name;
					current = &materials[ name ];
				}
				else if ( current )
				{
					if ( type == "Kd" ) input >> current->baseColour.r >> current->baseColour.g >> current->baseColour.b;
					else if ( type == "Ke" ) input >> current->emissive.r >> current->emissive.g >> current->emissive.b;
					else if ( type == "d" )
					{
						input >> current->baseColour.a;
						if ( current->baseColour.a < 1.0f ) current->alphaMode = AlphaMode3D::Blend;
					}
					else if ( type == "Tr" )
					{
						float transparency = 0.0f;
						input >> transparency;
						current->baseColour.a = 1.0f - transparency;
						if ( transparency > 0.0f ) current->alphaMode = AlphaMode3D::Blend;
					}
					else if ( type == "Pr" ) input >> current->roughness;
					else if ( type == "Pm" ) input >> current->metallic;
					else if ( type == "map_Kd" || type == "map_Bump" || type == "bump" || type == "map_Ke" )
					{
						std::string filename;
						std::getline( input >> std::ws, filename );
						if ( filename.empty() || filename[ 0 ] == '-' ) continue; // MTL texture options not supported.
						auto texture = Ref< Texture2D >::Create( ( file.parent_path() / filename ).string() );
						if ( type == "map_Kd" ) current->baseColourTexture = texture;
						else if ( type == "map_Ke" ) current->emissiveTexture = texture;
						else current->normalTexture = texture;
					}
				}
			}
		}

		struct PartBuilder
		{
			std::string name;
			std::string material;
			std::vector< MeshVertex3D > vertices;
			std::vector< uint32_t > indices;
			std::vector< bool > hasNormal;
			std::unordered_map< VertexKey, uint32_t, VertexKeyHash > indicesByKey;
		};

		void CompleteGeometry( PartBuilder& part )
		{
			std::vector< Vec3f > normals( part.vertices.size(), Vec3f( 0 ) );
			std::vector< Vec3f > tangents( part.vertices.size(), Vec3f( 0 ) );
			std::vector< Vec3f > bitangents( part.vertices.size(), Vec3f( 0 ) );
			for ( size_t i = 0; i < part.indices.size(); i += 3 )
			{
				const auto a = part.indices[ i ], b = part.indices[ i + 1 ], c = part.indices[ i + 2 ];
				const Vec3f edge1 = part.vertices[ b ].position - part.vertices[ a ].position;
				const Vec3f edge2 = part.vertices[ c ].position - part.vertices[ a ].position;
				const Vec3f face = glm::cross( edge1, edge2 );
				for ( uint32_t index : { a, b, c } ) normals[ index ] += face;
				const Vec2f duv1 = part.vertices[ b ].texCoord - part.vertices[ a ].texCoord;
				const Vec2f duv2 = part.vertices[ c ].texCoord - part.vertices[ a ].texCoord;
				const float det = duv1.x * duv2.y - duv1.y * duv2.x;
				if ( std::abs( det ) < 1e-8f ) continue;
				const Vec3f tangent = ( edge1 * duv2.y - edge2 * duv1.y ) / det;
				const Vec3f bitangent = ( edge2 * duv1.x - edge1 * duv2.x ) / det;
				for ( uint32_t index : { a, b, c } ) { tangents[ index ] += tangent; bitangents[ index ] += bitangent; }
			}
			for ( size_t i = 0; i < part.vertices.size(); ++i )
			{
				auto& vertex = part.vertices[ i ];
				if ( !part.hasNormal[ i ] ) vertex.normal = glm::length( normals[ i ] ) > 1e-8f ? glm::normalize( normals[ i ] ) : Vec3f( 0, 1, 0 );
				Vec3f tangent = tangents[ i ] - vertex.normal * glm::dot( vertex.normal, tangents[ i ] );
				if ( glm::length( tangent ) < 1e-8f )
					tangent = glm::cross( std::abs( vertex.normal.y ) < .99f ? Vec3f( 0, 1, 0 ) : Vec3f( 1, 0, 0 ), vertex.normal );
				tangent = glm::normalize( tangent );
				vertex.tangent = Vec4f( tangent, glm::dot( glm::cross( vertex.normal, tangent ), bitangents[ i ] ) < 0 ? -1.0f : 1.0f );
			}
		}
	} // namespace

	Ref< Model3D > LoadOBJ( const std::filesystem::path& path )
	{
		std::ifstream input( path );
		if ( !input ) throw std::runtime_error( "Could not open OBJ: " + path.string() );
		auto model = Ref< Model3D >::Create();
		std::vector< Vec3f > positions, normals;
		std::vector< Vec2f > texCoords;
		MaterialMap materials;
		PartBuilder current;
		current.name = path.stem().string();
		const auto flush = [&]() {
			if ( current.indices.empty() ) return;
			CompleteGeometry( current );
			Material3D material;
			const auto found = materials.find( current.material );
			if ( found != materials.end() ) material = found->second;
			model->parts.push_back( { Ref< Mesh3D >::Create( current.vertices, current.indices ), material, current.name } );
			current.vertices.clear(); current.indices.clear(); current.hasNormal.clear(); current.indicesByKey.clear();
		};
		std::string line;
		while ( std::getline( input, line ) )
		{
			std::istringstream stream( line );
			std::string tag;
			stream >> tag;
			if ( tag == "v" ) { Vec3f v; if ( stream >> v.x >> v.y >> v.z ) positions.push_back( v ); }
			else if ( tag == "vt" ) { Vec2f v; if ( stream >> v.x >> v.y ) texCoords.push_back( v ); }
			else if ( tag == "vn" ) { Vec3f v; if ( stream >> v.x >> v.y >> v.z ) normals.push_back( glm::length( v ) > 1e-8f ? glm::normalize( v ) : Vec3f( 0, 1, 0 ) ); }
			else if ( tag == "mtllib" ) { std::string file; while ( stream >> file ) ReadMaterials( path.parent_path() / file, materials ); }
			else if ( tag == "o" || tag == "g" ) { flush(); stream >> current.name; }
			else if ( tag == "usemtl" ) { flush(); stream >> current.material; }
			else if ( tag == "f" )
			{
				std::vector< uint32_t > face;
				std::string item;
				while ( stream >> item )
				{
					if ( item[ 0 ] == '#' ) break;
					VertexKey key = ParseKey( item );
					key.position = ResolveIndex( key.position, positions.size() ) + 1;
					if ( key.texCoord ) key.texCoord = ResolveIndex( key.texCoord, texCoords.size() ) + 1;
					if ( key.normal ) key.normal = ResolveIndex( key.normal, normals.size() ) + 1;
					const auto it = current.indicesByKey.find( key );
					if ( it != current.indicesByKey.end() ) { face.push_back( it->second ); continue; }
					MeshVertex3D vertex;
					vertex.position = positions.at( ResolveIndex( key.position, positions.size() ) );
					if ( key.texCoord ) vertex.texCoord = texCoords.at( ResolveIndex( key.texCoord, texCoords.size() ) );
					if ( key.normal ) vertex.normal = normals.at( ResolveIndex( key.normal, normals.size() ) );
					const auto index = static_cast< uint32_t >( current.vertices.size() );
					current.vertices.push_back( vertex );
					current.hasNormal.push_back( key.normal != 0 );
					current.indicesByKey.emplace( key, index );
					face.push_back( index );
				}
				for ( size_t i = 1; i + 1 < face.size(); ++i )
				{ current.indices.push_back( face[ 0 ] ); current.indices.push_back( face[ i ] ); current.indices.push_back( face[ i + 1 ] ); }
			}
		}
		flush();
		return model;
	}

} // namespace sl
