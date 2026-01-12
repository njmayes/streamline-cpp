#pragma once

#include "sl/Core/Common/Base.h"

#include <charconv>
#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace sl {
	// ------------------------------------------------------------
	// CommandLine
	// ------------------------------------------------------------

	class CommandLineArgs
	{
	public:
		CommandLineArgs() = default;

		CommandLineArgs( int argc, char const* const* argv )
		{
			mArgs.reserve( argc > 0 ? static_cast< std::size_t >( argc ) : 0 );
			for ( int i = 0; i < argc; ++i )
			{
				mArgs.emplace_back( argv[ i ] ? std::string_view{ argv[ i ] } : std::string_view{} );
			}
		}

		std::size_t Count() const
		{
			return mArgs.size();
		}

		std::span< std::string_view const > Values() const
		{
			if ( mArgs.size() <= 1 )
			{
				return {};
			}
			return std::span< std::string_view const >( mArgs.data() + 1, mArgs.size() - 1 );
		}

		std::string_view operator[]( std::size_t idx ) const
		{
			return mArgs[ idx ];
		}

		auto begin() const
		{
			return mArgs.begin();
		}

		auto end() const
		{
			return mArgs.end();
		}

	private:
		std::vector< std::string_view > mArgs{};
	};

	// ------------------------------------------------------------
	// Utilities
	// ------------------------------------------------------------

	namespace detail {

		inline bool StartsWith( std::string_view s, std::string_view prefix )
		{
			return s.size() >= prefix.size() && s.substr( 0, prefix.size() ) == prefix;
		}

		inline std::optional< std::pair< std::string_view, std::string_view > > SplitOnce( std::string_view s, char ch )
		{
			auto const pos = s.find( ch );
			if ( pos == std::string_view::npos )
			{
				return std::nullopt;
			}
			return std::pair{ s.substr( 0, pos ), s.substr( pos + 1 ) };
		}

		inline std::optional< bool > ParseBool( std::string_view s )
		{
			if ( s.empty() || s == "1" || s == "true" || s == "TRUE" || s == "True" || s == "yes" || s == "on" )
			{
				return true;
			}
			if ( s == "0" || s == "false" || s == "FALSE" || s == "False" || s == "no" || s == "off" )
			{
				return false;
			}
			return std::nullopt;
		}

		template < typename T >
		inline std::optional< T > ParseNumber( std::string_view s )
		{
			T out{};

			auto const* b = s.data();
			auto const* e = s.data() + s.size();

			if constexpr ( std::is_integral_v< T > )
			{
				auto r = std::from_chars( b, e, out );
				if ( r.ec == std::errc{} && r.ptr == e )
				{
					return out;
				}
				return std::nullopt;
			}
			else if constexpr ( std::is_floating_point_v< T > )
			{
#if defined( __cpp_lib_to_chars ) && __cpp_lib_to_chars >= 201611L
				auto r = std::from_chars( b, e, out, std::chars_format::general );
				if ( r.ec == std::errc{} && r.ptr == e )
				{
					return out;
				}
				return std::nullopt;
#else
				( void )b;
				( void )e;
				return std::nullopt;
#endif
			}
			else
			{
				return std::nullopt;
			}
		}

		template < typename T >
		inline std::optional< T > ParseValue( std::string_view s )
		{
			if constexpr ( std::is_same_v< T, std::string_view > )
			{
				return s;
			}
			else if constexpr ( std::is_same_v< T, bool > )
			{
				return ParseBool( s );
			}
			else if constexpr ( std::is_arithmetic_v< T > )
			{
				return ParseNumber< T >( s );
			}
			else
			{
				return std::nullopt;
			}
		}
	} // namespace detail

	// ------------------------------------------------------------
	// Struct binding
	// ------------------------------------------------------------

	enum class Arity
	{
		Flag,	   // --verbose, -v, --verbose=false
		Value,	   // --threads 8, --threads=8, -t8, -t 8, -t=8
		Positional // consumed in declaration order
	};

	template < typename T, typename MemberT >
	using MemberPtr = MemberT T::*;

	template < typename MemberT >
	struct DefaultParser
	{
		std::optional< MemberT > operator()( std::string_view s ) const
		{
			return detail::ParseValue< MemberT >( s );
		}
	};

	template < typename T >
	struct FieldSpecBase
	{
		std::string_view long_name{};
		char short_name{};
		bool required{};
		Arity arity_kind{};
	};

	template < typename T, typename MemberT, typename ParserT >
	struct FieldSpec : FieldSpecBase< T >
	{
		MemberPtr< T, MemberT > member{};
		ParserT parser{};
		std::optional< MemberT > default_value{};
	};

	template < typename T, typename OwnerT, typename MemberT, typename ParserT = DefaultParser< MemberT > >
		requires std::is_base_of_v< OwnerT, T >
	inline auto Field(
		std::string_view long_name,
		char short_name,
		MemberT OwnerT::* member,
		Arity arity = Arity::Value,
		ParserT parser = {}
	)
	{
		FieldSpec< T, MemberT, ParserT > f{};
		f.long_name = long_name;
		f.short_name = short_name;
		f.required = true;
		f.arity_kind = arity;

		// Convert base member pointer to T member pointer (now not part of deduction)
		f.member = static_cast< MemberT T::* >( member );

		f.parser = std::move( parser );
		return f;
	}

	template < typename T, typename OwnerT, typename MemberT, typename ParserT = DefaultParser< MemberT >, std::convertible_to< MemberT > DefaultT >
		requires std::is_base_of_v< OwnerT, T >
	inline auto Field(
		std::string_view long_name,
		char short_name,
		MemberT OwnerT::* member,
		DefaultT const& default_value,
		Arity arity = Arity::Value,
		ParserT parser = {}
	)
	{
		FieldSpec< T, MemberT, ParserT > f{};
		f.long_name = long_name;
		f.short_name = short_name;
		f.required = false;
		f.arity_kind = arity;

		// Convert base member pointer to T member pointer (now not part of deduction)
		f.member = static_cast< MemberT T::* >( member );

		f.default_value = std::move( default_value );
		f.parser = std::move( parser );
		return f;
	}

	struct ReadError
	{
		enum class Code
		{
			Ok,
			UnknownOption,
			MissingValue,
			BadValue,
			MissingRequired,
			TooManyPositionals
		};

		Code error_code{ Code::Ok };
		std::string_view token{};
		std::string_view field{};
	};

	template < typename T >
	using ReadResult = std::expected< T, ReadError >;

	namespace detail {

		template < typename SpecT >
		inline bool MatchesLong( SpecT const& spec, std::string_view name )
		{
			return !spec.long_name.empty() && spec.long_name == name;
		}

		template < typename SpecT >
		inline bool MatchesShort( SpecT const& spec, char c )
		{
			return spec.short_name != '\0' && spec.short_name == c;
		}

		template < typename T, typename SpecT >
		inline void ApplyDefault( T& out, SpecT const& spec )
		{
			if ( spec.default_value )
			{
				out.*( spec.member ) = *spec.default_value;
			}
		}

		template < typename T, typename SpecT >
		inline std::optional< ReadError > Assign( T& out, SpecT const& spec, std::string_view token, std::string_view value )
		{
			if ( spec.arity_kind == Arity::Flag )
			{
				using member_t = std::remove_reference_t< decltype( out.*( spec.member ) ) >;

				if constexpr ( std::is_same_v< member_t, bool > )
				{
					auto parsed = spec.parser( value );
					if ( !parsed )
					{
						return ReadError{ ReadError::Code::BadValue, token, spec.long_name };
					}
					out.*( spec.member ) = *parsed;
					return std::nullopt;
				}
				else
				{
					return ReadError{ ReadError::Code::BadValue, token, spec.long_name };
				}
			}

			auto parsed = spec.parser( value );
			if ( !parsed )
			{
				return ReadError{ ReadError::Code::BadValue, token, spec.long_name };
			}
			out.*( spec.member ) = *parsed;
			return std::nullopt;
		}
		template < typename T, typename... SpecsT >
		inline ReadResult< T > ReadImpl( CommandLineArgs const& cmd, std::tuple< SpecsT... > const& specs )
		{
			T out{};

			std::apply( [ & ]( auto const&... s ) { ( ApplyDefault( out, s ), ... ); }, specs );

			auto assigned = std::array< bool, sizeof...( SpecsT ) >{};
			assigned.fill( false );

			auto MarkAssigned = [ & ]( void const* spec_ptr ) {
				std::size_t idx = 0;
				std::apply(
					[ & ]( auto const&... s ) {
						( ( [ & ] {
							  if ( &s == spec_ptr )
							  {
								  assigned[ idx ] = true;
							  }
						  }(),
							++idx ),
						  ... );
					},
					specs
				);
			};

			auto FindLong = [ & ]( std::string_view name ) -> void const* {
				void const* found = nullptr;
				std::apply(
					[ & ]( auto const&... s ) {
						( ( [ & ] {
							  if ( !found && MatchesLong( s, name ) )
							  {
								  found = &s;
							  }
						  }() ),
						  ... );
					},
					specs
				);
				return found;
			};

			auto FindShort = [ & ]( char c ) -> void const* {
				void const* found = nullptr;
				std::apply(
					[ & ]( auto const&... s ) {
						( ( [ & ] {
							  if ( !found && MatchesShort( s, c ) )
							  {
								  found = &s;
							  }
						  }() ),
						  ... );
					},
					specs
				);
				return found;
			};

			auto const tokens = cmd.Values();

			std::vector< void const* > positional_specs{};
			positional_specs.reserve( sizeof...( SpecsT ) );
			std::apply(
				[ & ]( auto const&... s ) {
					( ( [ & ] {
						  if ( s.arity_kind == Arity::Positional )
						  {
							  positional_specs.push_back( &s );
						  }
					  }() ),
					  ... );
				},
				specs
			);

			std::size_t positional_idx = 0;

			for ( std::size_t i = 0; i < tokens.size(); ++i )
			{
				auto const tok = tokens[ i ];

				if ( StartsWith( tok, "--" ) )
				{
					auto body = tok.substr( 2 );

					std::string_view name = body;
					std::string_view value{};
					bool has_inline_value = false;

					if ( auto sv = SplitOnce( body, '=' ) )
					{
						name = sv->first;
						value = sv->second;
						has_inline_value = true;
					}

					auto const* spec_ptr = FindLong( name );
					if ( !spec_ptr )
					{
						return std::unexpected( ReadError{ ReadError::Code::UnknownOption, tok, name } );
					}

					std::optional< ReadError > err{};
					std::apply(
						[ & ]( auto const&... s ) {
							( ( [ & ] {
								  if ( &s != spec_ptr )
								  {
									  return;
								  }

								  if ( s.arity_kind == Arity::Flag )
								  {
									  err = Assign( out, s, tok, has_inline_value ? value : std::string_view{} );
									  MarkAssigned( spec_ptr );
									  return;
								  }

								  if ( has_inline_value )
								  {
									  err = Assign( out, s, tok, value );
									  MarkAssigned( spec_ptr );
									  return;
								  }

								  if ( i + 1 >= tokens.size() )
								  {
									  err = ReadError{ ReadError::Code::MissingValue, tok, name };
									  return;
								  }

								  ++i;
								  err = Assign( out, s, tok, tokens[ i ] );
								  MarkAssigned( spec_ptr );
							  }() ),
							  ... );
						},
						specs
					);

					if ( err )
					{
						return std::unexpected( *err );
					}
					continue;
				}

				if ( StartsWith( tok, "-" ) && tok.size() >= 2 )
				{
					auto body = tok.substr( 1 );
					char const opt = body[ 0 ];

					std::string_view value{};
					bool has_inline_value = false;

					if ( body.size() >= 3 && body[ 1 ] == '=' )
					{
						value = body.substr( 2 );
						has_inline_value = true;
					}
					else if ( body.size() > 1 )
					{
						value = body.substr( 1 );
						has_inline_value = true;
					}

					auto const* spec_ptr = FindShort( opt );
					if ( !spec_ptr )
					{
						return std::unexpected( ReadError{ ReadError::Code::UnknownOption, tok, std::string_view{ &opt, 1 } } );
					}

					std::optional< ReadError > err{};
					std::apply(
						[ & ]( auto const&... s ) {
							( ( [ & ] {
								  if ( &s != spec_ptr )
								  {
									  return;
								  }

								  if ( s.arity_kind == Arity::Flag )
								  {
									  err = Assign( out, s, tok, has_inline_value ? value : std::string_view{} );
									  MarkAssigned( spec_ptr );
									  return;
								  }

								  if ( has_inline_value )
								  {
									  err = Assign( out, s, tok, value );
									  MarkAssigned( spec_ptr );
									  return;
								  }

								  if ( i + 1 >= tokens.size() )
								  {
									  err = ReadError{ ReadError::Code::MissingValue, tok, s.long_name };
									  return;
								  }

								  ++i;
								  err = Assign( out, s, tok, tokens[ i ] );
								  MarkAssigned( spec_ptr );
							  }() ),
							  ... );
						},
						specs
					);

					if ( err )
					{
						return std::unexpected( *err );
					}
					continue;
				}

				if ( positional_idx >= positional_specs.size() )
				{
					return std::unexpected( ReadError{ ReadError::Code::TooManyPositionals, tok, {} } );
				}

				auto const* spec_ptr = positional_specs[ positional_idx ];
				std::optional< ReadError > err{};

				std::apply(
					[ & ]( auto const&... s ) {
						( ( [ & ] {
							  if ( &s != spec_ptr )
							  {
								  return;
							  }
							  err = Assign( out, s, tok, tok );
							  MarkAssigned( spec_ptr );
						  }() ),
						  ... );
					},
					specs
				);

				if ( err )
				{
					return std::unexpected( *err );
				}

				++positional_idx;
			}

			bool missing = false;
			std::string_view missing_field{};

			std::size_t idx = 0;
			std::apply(
				[ & ]( auto const&... s ) {
					( ( [ & ] {
						  if ( s.required && !assigned[ idx ] && !missing )
						  {
							  missing = true;
							  missing_field = s.long_name;
						  }
						  ++idx;
					  }() ),
					  ... );
				},
				specs
			);

			if ( missing )
			{
				return std::unexpected( ReadError{ ReadError::Code::MissingRequired, {}, missing_field } );
			}

			return out;
		}

	} // namespace detail

	template < typename T, typename... SpecsT >
	inline ReadResult< T > Read( CommandLineArgs const& cmd, SpecsT const&... specs )
	{
		return detail::ReadImpl< T >( cmd, std::tuple{ specs... } );
	}
} // namespace sl
