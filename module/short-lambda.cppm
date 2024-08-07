module;

#include "config.hpp"
#include "macros.hpp"

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <utility>


export module short_lambda;


export namespace short_lambda::details {
  template < class T, class U /*shall have no cvref*/ >
  concept similar_to = std::same_as< U, std::remove_cvref_t< T > >;

  template < class T, template < class... > class U, class... Us >
  concept satisfy = U< std::remove_cvref_t< T >, Us... >::value;

  template < template < class... > class U, class... Ts >
  concept any_satisfy = ( U< std::remove_cvref_t< Ts > >::value || ... );

  template < template < class... > class U, class... Ts >
  concept all_satisfy = ( U< std::remove_cvref_t< Ts > >::value && ... );


  template < class T, class U > [[nodiscard]] constexpr auto&& forward_like( U&& x ) noexcept {
    constexpr bool is_adding_const = std::is_const_v< std::remove_reference_t< T > >;
    if constexpr ( std::is_lvalue_reference_v< T&& > ) {
      if constexpr ( is_adding_const ) {
        return std::as_const( x );
      } else {
        return static_cast< U& >( x );
      }
    } else if constexpr ( is_adding_const ) {
      return std::move( std::as_const( x ) );
    } else {
      return std::move( x );
    }
  }

  template < class T > struct tuple_leaf_impl {
    T value;
  };

  template < std::size_t id, class T > struct tuple_leaf: tuple_leaf_impl< T > {
    template < class U >
      requires std::is_constructible_v< T, U&& >
    constexpr tuple_leaf( U&& u )
        noexcept( noexcept( tuple_leaf_impl< T >{ std::forward< U >( u ) } ) )
      : tuple_leaf_impl< T >{ std::forward< U >( u ) } { }
  };

  template < class, class... Ts > struct tuple_impl;

  template < std::size_t... Is, class... Ts >
  struct tuple_impl< std::index_sequence< Is... >, Ts... >: tuple_leaf< Is, Ts >... {
    template < class... Us >
      requires ( sizeof...( Us ) == sizeof...( Ts )
                 && ( std::is_constructible_v< Ts, Us && > && ... ) )
    constexpr tuple_impl( Us&&... us )
        noexcept( noexcept( ( (void) tuple_leaf< Is, Ts >( std::forward< Us >( us ) ), ... ) ) )
      : tuple_leaf< Is, Ts >( std::forward< Us >( us ) )... { }
  };
  template < class... Ts > struct tuple: tuple_impl< std::index_sequence_for< Ts... >, Ts... > {
    template < class... Us >
      requires ( sizeof...( Us ) == sizeof...( Ts )
                 && ( std::is_constructible_v< Ts, Us && > && ... ) )
    constexpr tuple( Us&&... us ) noexcept( noexcept(
        tuple_impl< std::index_sequence_for< Ts... >, Ts... >( std::forward< Us >( us )... ) ) )
      : tuple_impl< std::index_sequence_for< Ts... >, Ts... >( std::forward< Us >( us )... ) { }
  };

  template < class... Ts > tuple( Ts&&... ) -> tuple< Ts... >;

  template < class > struct is_tuple: std::false_type { };
  template < class... Ts > struct is_tuple< tuple< Ts... > >: std::true_type { };
  template < std::size_t, class > struct tuple_element { };
  template < std::size_t I, class R > struct tuple_element< I, R const > {
    using type [[maybe_unused]] = std::add_const_t< typename tuple_element< I, R >::type >;
  };
  template < std::size_t idx, class Tuple >
  using tuple_element_t = typename tuple_element< idx, Tuple >::type;

#if ! defined( __cpp_pack_indexing )
  template < std::size_t I, class T, class... Ts >
    requires ( I <= sizeof...( Ts ) )
  struct tuple_element< I, tuple< T, Ts... > >: tuple_element< I - 1, tuple< Ts... > > { };

  template < class T, class... Ts > struct tuple_element< 0, tuple< T, Ts... > > {
    using type = T;
  };
#else
  template < std::size_t I, class... Ts > struct tuple_element< I, tuple< Ts... > > {
    using type = Ts...[ I ];
  };
#endif
  template < class... Ts >
  auto make_tuple( Ts&&... args ) SL_expr_equiv( tuple< Ts... >( std::forward< Ts >( args )... ) )

  template < class... Ts >
  auto forward_as_tuple( Ts&&... args )
      SL_expr_equiv( tuple< Ts&&... >( std::forward< Ts >( args )... ) )

  template < std::size_t id, satisfy< is_tuple > Tuple >
  auto get( Tuple&& tuple ) SL_expr_equiv(
      static_cast< typename tuple_element< id, std::remove_cvref_t< Tuple > >::type&& >(
          tuple.tuple_leaf< id,
                            typename tuple_element< id, std::remove_cvref_t< Tuple > >::type >::value ) )
} // namespace short_lambda::details


export namespace short_lambda {
  template < class CallableT > struct lambda;

  template < class T > struct is_short_lambda: std::false_type { };

  template < class ClosureT > struct is_short_lambda< lambda< ClosureT > >: std::true_type { };

  enum class operators : unsigned int {
    plus = 0,
    minus,
    multiply,
    divide,
    modulus,
    negate,
    positate = 6,
    bit_and  = 7,
    bit_or,
    bit_xor,
    left_shift,
    right_shift = 10, // ^ arithmetic
    logical_and = 11,
    logical_or,
    logical_not = 13, // ^ logical
    equal_to    = 14,
    not_equal_to,
    greater,
    less,
    greater_equal,
    less_equal,
    compare_three_way = 20, // ^ comparison
    post_increment    = 21,
    pre_increment,
    post_decrement,
    pre_decrement = 24, // ^ in/de-crement
    assign_to     = 25,
    add_to,
    subtract_from,
    times_by,
    divide_by,
    modulus_with,
    bit_or_with,
    bit_and_with,
    bit_xor_with,
    left_shift_with,
    right_shift_with = 35, // ^ assignment
    function_call    = 36,
    comma,
    conditional = 38, // ^ misc
    subscript   = 39,
    address_of  = 40,
    indirection,
    object_member_access, // TBD
    pointer_member_access,
    object_member_access_of_pointer,
    pointer_member_access_of_pointer = 45, // ^ member access
    static_cast_                     = 46,
    dynamic_cast_,
    const_cast_,
    reinterpret_cast_,
    cstyle_cast, // cstyle cast, not keyword.
    sizeof_,
    alignof_,
    decltype_,
    typeid_,
    throw_,
    noexcept_,
    new_,           // TBD
    delete_,        // TBD
    co_await_ = 59, // ^ special, v extra, provide by me
    then      = 60, // expression-equivalent to `(void)lhs, rhs`
    typeof_,        // typeof_(x) := decltype((x)), they are c23 keyword, so we add a _ suffix
    typeof_unqual_  // typeof_unqual_(x) := std::remove_cvref_t<decltype((x))>
  };

  template < operators op > struct operators_t {
    SL_using_v value = op;
  };

  template < class T > struct is_operators_t: std::false_type { };

  template < operators op > struct is_operators_t< operators_t< op > >: std::true_type { };

  template < class T > struct every_operator_with_lambda_enabled: std::false_type { };

  template < details::satisfy< is_short_lambda > lambdaT >
  struct every_operator_with_lambda_enabled< lambdaT >: std::true_type { };

  template < class T, details::satisfy< is_operators_t > OpT >
  struct operator_with_lambda_enabled: std::false_type { };

  template < details::satisfy< every_operator_with_lambda_enabled > T,
             details::satisfy< is_operators_t >                     OpT >
  struct operator_with_lambda_enabled< T, OpT >: std::true_type { };

  namespace function_object {

#define SL_define_binary_op( name, op )                                                            \
  struct name##_t {                                                                                \
    template < class LHS, class RHS >                                                              \
    SL_using_c operator( )( LHS&& lhs, RHS&& rhs ) SL_expr_equiv(                                  \
        std::forward< LHS >( lhs ) SL_remove_parenthesis( op ) std::forward< RHS >( rhs ) )        \
  } SL_using_st( name ){ };

    SL_define_binary_op( plus, ( +) )
    SL_define_binary_op( minus, ( -) )
    SL_define_binary_op( multiply, (*) )
    SL_define_binary_op( divide, ( / ) )
    SL_define_binary_op( modulus, ( % ) )

    SL_define_binary_op( bit_and, (&) )
    SL_define_binary_op( bit_or, ( | ) )
    SL_define_binary_op( bit_xor, ( ^) )
    SL_define_binary_op( left_shift, ( << ) )
    SL_define_binary_op( right_shift, ( >> ) )

    SL_define_binary_op( logical_and, (&&) )
    SL_define_binary_op( logical_or, ( || ) )

    SL_define_binary_op( equal_to, ( == ) )
    SL_define_binary_op( not_equal_to, ( != ) )
    SL_define_binary_op( greater, ( > ) )
    SL_define_binary_op( less, ( < ) )
    SL_define_binary_op( greater_equal, ( >= ) )
    SL_define_binary_op( less_equal, ( <= ) )
    SL_define_binary_op( compare_three_way, ( <=> ) )

    SL_define_binary_op( comma, (, ) )

    SL_define_binary_op( pointer_member_access_of_pointer, (->*) )

    SL_define_binary_op( assign_to, ( = ) )
    SL_define_binary_op( add_to, ( += ) )
    SL_define_binary_op( subtract_from, ( -= ) )
    SL_define_binary_op( times_by, ( *= ) )
    SL_define_binary_op( divide_by, ( /= ) )
    SL_define_binary_op( modulus_with, ( %= ) )
    SL_define_binary_op( bit_or_with, ( |= ) )
    SL_define_binary_op( bit_and_with, ( &= ) )
    SL_define_binary_op( bit_xor_with, ( ^= ) )
    SL_define_binary_op( left_shift_with, ( <<= ) )
    SL_define_binary_op( right_shift_with, ( >>= ) )
#undef SL_define_binary_op // undefined


#define SL_define_unary_op( name, op )                                                             \
  struct name##_t {                                                                                \
    template < class Operand >                                                                     \
    SL_using_c operator( )( Operand&& arg )                                                        \
        SL_expr_equiv( SL_remove_parenthesis( op ) std::forward< Operand >( arg ) )                \
  } SL_using_st( name ){ };

    SL_define_unary_op( negate, ( -) )
    SL_define_unary_op( positate, ( +) )
    SL_define_unary_op( bit_not, ( ~) )
    SL_define_unary_op( logical_not, ( ! ) )
    SL_define_unary_op( address_of, (&) )
    SL_define_unary_op( indirection, (*) )
    SL_define_unary_op( pre_increment, ( ++) )
    SL_define_unary_op( pre_decrement, ( --) )

#undef SL_define_unary_op

    struct post_increment_t {
      template < class Operand >
      SL_using_c operator( )( Operand&& arg ) SL_expr_equiv( std::forward< Operand >( arg )-- )
    } SL_using_st( post_increment ){ };

    struct post_decrement_t {
      template < class Operand >
      SL_using_c operator( )( Operand&& arg ) SL_expr_equiv( std::forward< Operand >( arg )-- )
    } SL_using_st( post_decrement ){ };

    struct object_member_access_of_pointer_t {
      template < class Operand >
      SL_using_c operator( )( Operand&& arg )
          SL_expr_equiv( std::forward< Operand >( arg ).operator->( ) )
    } SL_using_st( object_member_access_of_pointer ){ };


    // some unoverloadable operator

    struct pointer_member_access_t { // a.*b
      template < class LHS, class RHS >
      SL_using_c operator( )( LHS&& lhs, RHS&& rhs )
          SL_expr_equiv( std::forward< LHS >( lhs ).*( std::forward< RHS >( rhs ) ) )
    } SL_using_st( pointer_member_access ){ };

    // It seems that it's impossible to implement object_member_access (a.k.a. `dot') operator.

    struct function_call_t {
      template < class F, class... Args >
      SL_using_c operator( )( F&& f, Args&&... args )
          SL_expr_equiv( std::forward< F >( f )( std::forward< Args >( args )... ) )
    } SL_using_st( function_call ){ };

#if not ( defined( SL_cxx_msvc ) or defined( SL_anal_resharper ) )
    /// @note: msvc does not support multiple index subscript operator
    ///        resharper++ obeys msvc's prefer.
    struct subscript_t {
      template < class Array, class... Idx >
      SL_using_c operator( )( Array&& arr, Idx&&... idx )
          SL_expr_equiv( std::forward< Array >( arr )[ std::forward< Idx >( idx )... ] )
    } SL_using_st( subscript ){ };
#else
    struct subscript_t {
      template < class Array, class Idx >
      SL_using_c operator( )( Array&& arr, Idx&& idx )
          SL_expr_equiv( std::forward< Array >( arr )[ std::forward< Idx >( idx ) ] )
    } SL_using_st( subscript ){ };
#endif

    struct conditional_t {
      template < class Cond, class TrueB, class FalseB >
      SL_using_c operator( )( Cond&& cond, TrueB&& trueb, FalseB&& falseb )
          SL_expr_equiv( std::forward< Cond >( cond ) ? std::forward< TrueB >( trueb )
                                                      : std::forward< FalseB >( falseb ) )
    } SL_using_st( conditional ){ };

    struct static_cast_t {
      template < class Target, class Op >
      SL_using_c operator( )( Op&& arg, std::type_identity< Target > = { } )
          SL_expr_equiv( static_cast< Target >( std::forward< Op >( arg ) ) )
    } SL_using_st( static_cast_ ){ };

    struct const_cast_t {
      template < class Target, class Op >
      SL_using_c operator( )( Op&& arg, std::type_identity< Target > = { } )
          SL_expr_equiv( const_cast< Target >( std::forward< Op >( arg ) ) )
    } SL_using_st( const_cast_ ){ };

    struct dynamic_cast_t {
      template < class Target, class Op >
      SL_using_c operator( )( Op&& arg, std::type_identity< Target > = { } )
          SL_expr_equiv( dynamic_cast< Target >( std::forward< Op >( arg ) ) )
    } SL_using_st( dynamic_cast_ ){ };

    struct reinterpret_cast_t {
      template < class Target, class Op >
      SL_using_c operator( )( Op&& arg, std::type_identity< Target > = { } )
          SL_expr_equiv( reinterpret_cast< Target >( std::forward< Op >( arg ) ) )
    } SL_using_st( reinterpret_cast_ ){ };

    struct cstyle_cast_t {
      template < class Target, class Op >
      SL_using_c operator( )( Op&& arg, std::type_identity< Target > = { } )
          SL_expr_equiv( (Target) ( std::forward< Op >( arg ) ) )
    } SL_using_st( cstyle_cast ){ };

    struct throw_t {
      template < class Op >
#if ! defined( SL_cxx_msvc )
      /*constexpr*/ [[noreturn]] static auto operator( )( Op&& arg ) noexcept( false ) -> void
#else
      /*constexpr*/ [[noreturn]] auto operator( )( Op&& arg ) noexcept( false ) -> void
#endif
        requires requires { auto{ std::forward< Op >( arg ) }; }
      {
        throw std::forward< Op >( arg );
      }
    } SL_using_st( throw_ ){ };

    struct noexcept_t {
      /// @note: this operator can not work as expected, so we delete it
      template < class Op > SL_using_c operator( )( Op&& arg ) noexcept -> bool = delete;
    } SL_using_st( noexcept_ ){ };

    struct decltype_t {
      template < class Op, bool id = false >
      SL_using_c operator( )( Op&& arg, std::integral_constant< bool, id > = { } ) noexcept
        requires ( ( id && requires { std::type_identity< decltype( arg ) >{ }; } )
                   || ( requires { std::type_identity< decltype( ( arg ) ) >{ }; } ) )
      {
        if constexpr ( id ) {
          return std::type_identity< decltype( arg ) >{ };
        } else {
          return std::type_identity< decltype( ( arg ) ) >{ };
        }
      }
    } SL_using_st( decltype_ ){ };

    struct typeid_t {
      template < class Op >
      SL_using_c operator( )( Op&& arg ) noexcept
        requires requires { std::type_index{ typeid( arg ) }; }
      {
        return std::type_index{ typeid( arg ) };
      }
    } SL_using_st( typeid_ ){ };

    struct sizeof_t {
      template < class Op >
      SL_using_c operator( )( Op&& arg ) noexcept
        requires requires { sizeof( arg ); }
      {
        return sizeof( arg );
      }
    } SL_using_st( sizeof_ ){ };

    struct alignof_t {
      template < class Op >
      SL_using_c operator( )( Op&& ) noexcept
        requires requires { alignof( std::remove_cvref_t< Op > ); }
      {
        return alignof( std::remove_cvref_t< Op > );
      }
    } SL_using_st( alignof_ ){ };

    struct new_t {
      template < class T0, class... Ts >
      SL_using_c operator( )( std::type_identity< T0 >, Ts&&... args )
          SL_expr_equiv( new T0{ std::forward< Ts >( args )... } )
    } SL_using_st( new_ ){ };

    struct delete_t {
      template < bool Array, class Op > static consteval inline bool constraint_of( ) noexcept {
        if constexpr ( Array ) {
          return requires { delete[] std::declval< Op >( ); };
        } else {
          return requires { delete std::declval< Op >( ); };
        }
      };
      template < bool Array, class Op > static consteval inline bool noexcept_of( ) noexcept {
        if constexpr ( Array ) {
          return noexcept( delete[] std::declval< Op >( ) );
        } else {
          return noexcept( delete std::declval< Op >( ) );
        }
      };

      template < bool Array = false, class Op >
      SL_using_c operator( )( Op&& arg, std::integral_constant< bool, Array > = { } )
          noexcept( noexcept_of< Array, Op >( ) )
              ->decltype( auto )
        requires ( constraint_of< Array, Op >( ) )
      {
        if constexpr ( Array ) {
          delete[] arg;
        } else {
          delete arg;
        }
      }
    } SL_using_st( delete_ ){ };

    struct co_await_t {
      template < class Op >
      SL_using_c operator( )( Op&& arg )
          noexcept( ( requires { std::forward< Op >( arg ).operator co_await( ); }
                      && noexcept( std::forward< Op >( arg ).operator co_await( ) ) )
                    || ( requires { operator co_await( std::forward< Op >( arg ) ); }
                         && noexcept( operator co_await( std::forward< Op >( arg ) ) ) ) )
              ->decltype( auto )
        requires (
            requires { std::forward< Op >( arg ).operator co_await( ); }
            || requires { operator co_await( std::forward< Op >( arg ) ); } )
      {
        if constexpr ( requires { std::forward< Op >( arg ).operator co_await( ); } ) {
          return std::forward< Op >( arg ).operator co_await( );
        } else if constexpr ( requires { operator co_await( std::forward< Op >( arg ) ); } ) {
          return operator co_await( std::forward< Op >( arg ) );
        } else {
          static_assert( false );
        }
      }
    } SL_using_st( co_await_ ){ };

    struct then_t {
      template < class LHS, class RHS >
      SL_using_c operator( )( LHS&& lhs, RHS&& rhs )
          noexcept( noexcept( lhs ) && noexcept( std::forward< RHS >( rhs ) ) )
              ->decltype( auto )
        requires requires {
          lhs;
          std::forward< RHS >( rhs );
        }
      {
        lhs;
        return std::forward< RHS >( rhs );
      }
    } SL_using_st( then ){ };

    struct typeof_t {
      template < class Op >
      SL_using_c operator( )( Op&& arg )
          SL_expr_equiv( std::type_identity< decltype( ( arg ) ) >{ } )
    } SL_using_st( typeof_ ){ };

    struct typeof_unqual_t {
      template < class Op >
      SL_using_c operator( )( Op&& arg )
          SL_expr_equiv( std::type_identity< std::remove_cvref_t< decltype( ( arg ) ) > >{ } )
    } SL_using_st( typeof_unqual_ ){ };


  } // namespace function_object

  inline namespace lambda_operators {
#define SL_lambda_binary_operator( name, op )                                                        \
  template < details::satisfy< operator_with_lambda_enabled, operators_t< operators::name > > LHS,   \
             details::satisfy< operator_with_lambda_enabled, operators_t< operators::name > > RHS >  \
    requires details::any_satisfy< is_short_lambda, LHS, RHS >                                       \
  SL_using_m operator SL_remove_parenthesis( op )( LHS&& lhs, RHS&& rhs ) SL_expr_equiv( lambda {    \
    [                                                                                                \
      lhs{ std::forward< LHS >( lhs ) },                                                             \
      rhs{ std::forward< RHS >( rhs ) }                                                              \
    ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )                  \
        SL_expr_equiv_declval( /*req*/ ( ::short_lambda::function_object::name(                      \
                                   SL_forward_like_app( std::declval< LHS >( ) ),                    \
                                   SL_forward_like_app( std::declval< RHS >( ) ) ) ),                \
                               ::short_lambda::function_object::name( SL_forward_like_app( lhs ),    \
                                                                      SL_forward_like_app( rhs ) ) ) \
  } )

    SL_lambda_binary_operator( plus, ( +) )
    SL_lambda_binary_operator( minus, ( -) )
    SL_lambda_binary_operator( multiply, (*) )
    SL_lambda_binary_operator( divide, ( / ) )
    SL_lambda_binary_operator( modulus, ( % ) )
    SL_lambda_binary_operator( bit_and, (&) )
    SL_lambda_binary_operator( bit_or, ( | ) )
    SL_lambda_binary_operator( bit_xor, ( ^) )
    SL_lambda_binary_operator( left_shift, ( << ) )
    SL_lambda_binary_operator( right_shift, ( >> ) )
    SL_lambda_binary_operator( logical_and, (&&) )
    SL_lambda_binary_operator( logical_or, ( || ) )
    SL_lambda_binary_operator( equal_to, ( == ) )
    SL_lambda_binary_operator( not_equal_to, ( != ) )
    SL_lambda_binary_operator( greater, ( > ) )
    SL_lambda_binary_operator( less, ( < ) )
    SL_lambda_binary_operator( greater_equal, ( >= ) )
    SL_lambda_binary_operator( less_equal, ( <= ) )
    SL_lambda_binary_operator( compare_three_way, ( <=> ) )
    SL_lambda_binary_operator( comma, (, ) )
    SL_lambda_binary_operator( pointer_member_access_of_pointer, (->*) )

#undef SL_lambda_binary_operator


#define SL_lambda_unary_operator( name, op )                                                        \
  template < details::satisfy< is_short_lambda > Operand >                                          \
  SL_using_m operator SL_remove_parenthesis( op )( Operand&& fs ) SL_expr_equiv( lambda {           \
    [fs{ std::forward< Operand >(                                                                   \
        fs ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )       \
        SL_expr_equiv_declval( /*req*/ ( ::short_lambda::function_object::name(                     \
                                   SL_forward_like_app( std::declval< Operand >( ) ) ) ),           \
                               ::short_lambda::function_object::name( SL_forward_like_app( fs ) ) ) \
  } )

    SL_lambda_unary_operator( negate, ( -) )
    SL_lambda_unary_operator( positate, ( +) )
    SL_lambda_unary_operator( bit_not, ( ~) )
    SL_lambda_unary_operator( logical_not, ( ! ) )
    SL_lambda_unary_operator( address_of, (&) )

#undef SL_lambda_unary_operator

  } // namespace lambda_operators

  template < class CallableT > struct lambda {
    CallableT storage;

    using type   = CallableT;
    using self_t = lambda< CallableT >;

    template < details::similar_to< lambda > Self, class... Ts >
    SL_using_m operator( )( this Self&& self, Ts&&... args ) SL_expr_equiv(
        details::forward_like< Self >( self.storage )( std::forward< Ts >( args )... ) )


    template < class Lmb,
               details::satisfy< operator_with_lambda_enabled, operators_t< operators::then > > RHS >
    SL_using_m then( this Lmb&& lmb, RHS&& rhs ) SL_expr_equiv( ::short_lambda::lambda{
        [ lhs{ std::forward< Lmb >( lmb ) },
          rhs{ std::forward< RHS >( rhs ) } ]< class Self, class... Ts >(
            [[maybe_unused]] this Self&& self,
            Ts&&... args ) noexcept( noexcept( SL_forward_like_app( std::declval< Lmb >( ) ) ) && noexcept( SL_forward_like_app( std::declval< RHS >( ) ) ) )
            -> decltype( auto )
          requires (
              requires { SL_forward_like_app( std::declval< Lmb >( ) ); }
              && requires { SL_forward_like_app( std::declval< RHS >( ) ); } )
        {
          SL_forward_like_app( lhs );
          return SL_forward_like_app( rhs );
        } } )

#define SL_lambda_member_variadic_op( name )                                                           \
  template < class Lmb,                                                                                \
             details::satisfy< operator_with_lambda_enabled, operators_t< operators::name > >... RHS > \
  SL_using_m name( this Lmb&& lmb, RHS&&... rhs ) SL_expr_equiv( ::short_lambda::lambda {              \
    [                                                                                                  \
      lhs{ std::forward< Lmb >( lmb ) },                                                               \
      ... rhs{ std::forward< RHS >( rhs ) }                                                            \
    ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )                    \
        SL_expr_equiv_declval(                                                                         \
            ( function_object::name( SL_forward_like_app( std::declval< Lmb >( ) ),                    \
                                     SL_forward_like_app( std::declval< RHS >( ) )... ) ),             \
            function_object::name( SL_forward_like_app( lhs ), SL_forward_like_app( rhs )... ) )       \
  } );


    SL_lambda_member_variadic_op( function_call )
    SL_lambda_member_variadic_op( subscript )
#undef SL_lambda_member_variadic_op

#define SL_lambda_member_binary_op_named( name )                                                    \
  template < class Lmb,                                                                             \
             details::satisfy< operator_with_lambda_enabled, operators_t< operators::name > > RHS > \
  SL_using_m name( this Lmb&& lmb, RHS&& rhs ) SL_expr_equiv( ::short_lambda::lambda {              \
    [                                                                                               \
      lhs{ std::forward< Lmb >( lmb ) },                                                            \
      rhs{ std::forward< RHS >( rhs ) }                                                             \
    ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )                 \
        SL_expr_equiv_declval(                                                                      \
            ( function_object::name( SL_forward_like_app( std::declval< Lmb >( ) ),                 \
                                     SL_forward_like_app( std::declval< RHS >( ) ) ) ),             \
            function_object::name( SL_forward_like_app( lhs ), SL_forward_like_app( rhs ) ) )       \
  } );

    SL_lambda_member_binary_op_named( assign_to ) // avoid overloading copy/move assign operator!

#undef SL_lambda_member_binary_op_named

#define SL_lambda_member_binary_op( name, op )                                                      \
  template < class Lmb,                                                                             \
             details::satisfy< operator_with_lambda_enabled, operators_t< operators::name > > RHS > \
  SL_using_m operator SL_remove_parenthesis( op )( this Lmb&& lmb, RHS&& rhs )                      \
      SL_expr_equiv( ::short_lambda::lambda {                                                       \
        [                                                                                           \
          lhs{ std::forward< Lmb >( lmb ) },                                                        \
          rhs{ std::forward< RHS >( rhs ) }                                                         \
        ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )             \
            SL_expr_equiv_declval(                                                                  \
                ( function_object::name( SL_forward_like_app( std::declval< Lmb >( ) ),             \
                                         SL_forward_like_app( std::declval< RHS >( ) ) ) ),         \
                function_object::name( SL_forward_like_app( lhs ), SL_forward_like_app( rhs ) ) )   \
      } );

    SL_lambda_member_binary_op( add_to, ( += ) )
    SL_lambda_member_binary_op( subtract_from, ( -= ) )
    SL_lambda_member_binary_op( times_by, ( *= ) )
    SL_lambda_member_binary_op( divide_by, ( /= ) )
    SL_lambda_member_binary_op( bit_and_with, ( &= ) )
    SL_lambda_member_binary_op( bit_or_with, ( |= ) )
    SL_lambda_member_binary_op( bit_xor_with, ( ^= ) )
    SL_lambda_member_binary_op( left_shift_with, ( <<= ) )
    SL_lambda_member_binary_op( right_shift_with, ( >>= ) )

#undef SL_lambda_member_binary_op

#define SL_lambda_member_cast_op_named( name )                                                     \
  template < class Target, class Lmb >                                                             \
  SL_using_m name( this Lmb&& lmb,                                                                 \
                   std::type_identity< Target > = { } ) SL_expr_equiv( ::short_lambda::lambda {    \
    [lhs{ std::forward< Lmb >(                                                                     \
        lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )     \
        SL_expr_equiv_declval(                                                                     \
            ( function_object::name( SL_forward_like_app( std::declval< Lmb >( ) ),                \
                                     std::type_identity< Target >{ } ) ),                          \
            function_object::name( SL_forward_like_app( lhs ), std::type_identity< Target >{ } ) ) \
  } );

    SL_lambda_member_cast_op_named( const_cast_ )
    SL_lambda_member_cast_op_named( static_cast_ )
    SL_lambda_member_cast_op_named( dynamic_cast_ )
    SL_lambda_member_cast_op_named( reinterpret_cast_ )
    SL_lambda_member_cast_op_named( cstyle_cast )

#undef SL_lambda_member_cast_op_named

#define SL_lambda_member_unary_op_named( name )                                                    \
  template < class Lmb >                                                                           \
  SL_using_m name( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {                        \
    [lhs{ std::forward< Lmb >(                                                                     \
        lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )     \
        SL_expr_equiv_declval(                                                                     \
            ( function_object::name( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),            \
            function_object::name( SL_forward_like_app( lhs ) ) )                                  \
  } );

    SL_lambda_member_unary_op_named( throw_ )
    SL_lambda_member_unary_op_named( typeid_ )
    SL_lambda_member_unary_op_named( sizeof_ )
    SL_lambda_member_unary_op_named( alignof_ )
    SL_lambda_member_unary_op_named( typeof_ )
    SL_lambda_member_unary_op_named( typeof_unqual_ )
    SL_lambda_member_unary_op_named( co_await_ )

#undef SL_lambda_member_unary_op_named

    template < bool Id = false, class Lmb >
    SL_using_m decltype_( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::decltype_( SL_forward_like_app( std::declval< Lmb >( ) ),
                                            std::integral_constant< bool, Id >{ } ) ),
              function_object::decltype_( SL_forward_like_app( lhs ),
                                          std::integral_constant< bool, Id >{ } ) )
    } );

    template < class Lmb >
    SL_using_m noexcept_( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lmb{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval( ( noexcept( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
                                 noexcept( SL_forward_like_app( lmb ) ) )
    } );

    template < class Lmb >
    SL_using_m operator++( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::pre_increment( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::pre_increment( SL_forward_like_app( lhs ) ) )
    } );
    template < class Lmb >
    SL_using_m operator++( this Lmb&& lmb, int ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::post_increment( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::post_increment( SL_forward_like_app( lhs ) ) )
    } );
    template < class Lmb >
    SL_using_m operator--( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::pre_decrement( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::pre_decrement( SL_forward_like_app( lhs ) ) )
    } );
    template < class Lmb >
    SL_using_m operator--( this Lmb&& lmb, int ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::post_decrement( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::post_decrement( SL_forward_like_app( lhs ) ) )
    } );

    template <
        class Lmb,
        details::satisfy< operator_with_lambda_enabled, operators_t< operators::conditional > > TB,
        details::satisfy< operator_with_lambda_enabled, operators_t< operators::conditional > > FB >
    SL_using_m conditional( this Lmb&& lmb, TB&& tb, FB&& fb )
        SL_expr_equiv( ::short_lambda::lambda {
          [
            lhs{ std::forward< Lmb >( lmb ) },
            tb{ std::forward< TB >( tb ) },
            fb{ std::forward< FB >( fb ) }
          ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self,
                                        [[maybe_unused]] Ts&&... args )
              SL_expr_equiv_declval(
                  ( function_object::conditional( SL_forward_like_app( std::declval< Lmb >( ) ),
                                                  SL_forward_like_app( std::declval< TB >( ) ),
                                                  SL_forward_like_app( std::declval< FB >( ) ) ) ),
                  function_object::conditional( SL_forward_like_app( lhs ),
                                                SL_forward_like_app( tb ),
                                                SL_forward_like_app( fb ) ) )
        } );

    template < class Lmb >
    SL_using_m operator*( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::indirection( SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::indirection( SL_forward_like_app( lhs ) ) )
    } );

    template < class Lmb >
    SL_using_m operator->( this Lmb&& lmb ) SL_expr_equiv( ::short_lambda::lambda {
      [lhs{ std::forward< Lmb >(
          lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
          SL_expr_equiv_declval(
              ( function_object::object_member_access_of_pointer(
                  SL_forward_like_app( std::declval< Lmb >( ) ) ) ),
              function_object::object_member_access_of_pointer( SL_forward_like_app( lhs ) ) )
    } );

    template < class Lmb,
               details::satisfy< operator_with_lambda_enabled,
                                 operators_t< operators::pointer_member_access > > Mptr >
    SL_using_m pointer_member_access( this Lmb&& lmb, Mptr&& mptr )
        SL_expr_equiv( ::short_lambda::lambda {
          [
            lhs{ std::forward< Lmb >( lmb ) },
            mptr{ std::forward< Mptr >( mptr ) }
          ]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
              SL_expr_equiv_declval(
                  ( function_object::pointer_member_access(
                      SL_forward_like_app( std::declval< Lmb >( ) ),
                      SL_forward_like_app( std::declval< Mptr >( ) ) ) ),
                  function_object::pointer_member_access( SL_forward_like_app( lhs ),
                                                          SL_forward_like_app( mptr ) ) )
        } );

    template < class T,
               details::satisfy< operator_with_lambda_enabled, operators_t< operators::new_ > >... Args >
    SL_using_f new_( std::type_identity< T >, Args&&... args1 ) noexcept( noexcept(
        ::short_lambda::lambda{ [... args1{ std::declval< Args >( ) } ]< class Self, class... Ts >(
                                    [[maybe_unused]] this Self&& self,
                                    [[maybe_unused]] Ts&&... args ) {
          return ( std::forward< Args >( args1 )( std::declval< Ts >( )... ), ... );
        } } ) ) -> decltype( auto )
      requires requires {
        ::short_lambda::lambda{
            [... args1{ std::declval< Args >( ) } ]< class Self, class... Ts >( this Self&&,
                                                                                Ts&&... ) {
              return ( std::forward< Args >( args1 )( std::declval< Ts >( )... ), ... );
            } };
      }
    {
      return ::short_lambda::lambda {
        [... args1{ std::forward< Args >(
            args1 ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
            SL_expr_equiv_declval(
                ( function_object::new_( std::type_identity< T >{ },
                                         SL_forward_like_app( std::declval< Args >( ) )... ) ),
                function_object::new_( std::type_identity< T >{ }, SL_forward_like_app( args1 )... ) )
      };
    }

    template < bool Array = false, class Lmb >
    SL_using_m delete_( this Lmb&& lmb )
        noexcept( noexcept( auto{ std::declval< Lmb >( ) } ) ) -> decltype( auto )
      requires requires { auto{ std::declval< Lmb >( ) }; }
    {
      return ::short_lambda::lambda {
        [lhs{ std::forward< Lmb >(
            lmb ) }]< class Self, class... Ts >( [[maybe_unused]] this Self&& self, Ts&&... args )
            SL_expr_equiv_declval(
                ( function_object::delete_( SL_forward_like_app( std::declval< Lmb >( ) ),
                                            std::integral_constant< bool, Array >{ } ) ),
                function_object::delete_( SL_forward_like_app( lhs ),
                                          std::integral_constant< bool, Array >{ } ) )
      };
    }
  };

  inline namespace factory {
    template < std::size_t idx > struct projector_t {
      template < class... Ts >
      constexpr inline static bool construct_from_input
          = not std::is_lvalue_reference_v< details::tuple_element_t< idx, details::tuple< Ts&&... > > >;


      template < class... Ts >
        requires ( sizeof...( Ts ) > idx )
      constexpr static typename std::conditional_t<
          construct_from_input< Ts... >,
          std::remove_cvref_t< details::tuple_element_t< idx, details::tuple< Ts... > > >,
          details::tuple_element_t< idx, details::tuple< Ts... > > >
      operator( )( Ts&&... args ) SL_expr_equiv_no_ret(
          details::get< idx >( details::make_tuple( std::forward< Ts >( args )... ) ) )
    };

    struct lift_t { // forwarding construct received argument
      template < class T > static bool consteval inline constraint_of( ) noexcept {
        if constexpr ( std::is_lvalue_reference_v< T&& > ) {
          return requires {
            ( lambda{ [ v = std::addressof( std::declval< T& >( ) ) ]< class Self, class... Ts >(
                          [[maybe_unused]] this Self&& self,
                          [[maybe_unused]] Ts&&... args ) noexcept -> decltype( auto ) {
              return static_cast< T >( *v );
            } } );
          };
        } else {
          return requires {
            ( lambda{ [ v{ std::declval< T& >( ) } ]< class Self, class... Ts >(
                          [[maybe_unused]] this Self&& self,
                          [[maybe_unused]] Ts&&... args ) noexcept( noexcept( auto{
                          std::declval< T& >( ) } ) ) -> auto { return v; } } );
          };
        }
      }
      template < class T > static bool consteval inline noexcept_of( ) noexcept {
        if constexpr ( std::is_lvalue_reference_v< T&& > ) {
          return noexcept(
              lambda{ [ v = std::addressof( std::declval< T& >( ) ) ]< class Self, class... Ts >(
                          [[maybe_unused]] this Self&& self,
                          [[maybe_unused]] Ts&&... args ) noexcept -> decltype( auto ) {
                return static_cast< T >( *v );
              } } );
        } else {
          return noexcept( lambda{ [ v{ std::declval< T& >( ) } ]< class Self, class... Ts >(
                                       [[maybe_unused]] this Self&& self,
                                       [[maybe_unused]] Ts&&... args ) noexcept( noexcept( auto{
                                       std::declval< T& >( ) } ) ) -> auto { return v; } } );
        }
      }

      template < class T >
      SL_using_c operator( )( T&& value ) noexcept( noexcept_of< T >( ) )
          ->decltype( auto )
        requires ( constraint_of< T >( ) )
      {
        /// @note: there's a bug in gcc:
        /// [[maybe_unused]] does not have effect here.
        if constexpr ( std::is_lvalue_reference_v< T&& > ) {
          return ( lambda{
              [ v = std::addressof( value ) ]< class Self, class... Ts >(
                  [[maybe_unused]] this Self&& self,
                  Ts&&... ) noexcept -> decltype( auto ) { return static_cast< T >( *v ); } } );
        } else {
          return ( lambda{ [ v{ std::forward< T >( value ) } ]< class Self, class... Ts >(
                               [[maybe_unused]] this Self&& self,
                               Ts&&... ) noexcept( noexcept( auto{
                               std::declval< T& >( ) } ) ) -> auto { return v; } } );
        }
      }
    };


    SL_using_m $0 = lambda{ projector_t< 0 >{} };
    SL_using_m $1 = lambda{ projector_t< 1 >{} };
    SL_using_m $2 = lambda{ projector_t< 2 >{} };
    SL_using_m $3 = lambda{ projector_t< 3 >{} };
    SL_using_m $4 = lambda{ projector_t< 4 >{} };
    SL_using_m $5 = lambda{ projector_t< 5 >{} };
    SL_using_m $6 = lambda{ projector_t< 6 >{} };
    SL_using_m $7 = lambda{ projector_t< 7 >{} };
    SL_using_m $8 = lambda{ projector_t< 8 >{} };
    SL_using_m $9 = lambda{ projector_t< 9 >{} };
    SL_using_m $  = lift_t{ };

    template < class U > struct coprojector_t {
      template < class T >
      SL_using_c operator( )( T&& arg ) SL_expr_equiv( $( static_cast< U& >( arg ) ) )
    };

    template < class T > struct storage_t {
      T value;

      template < class U, class Self >
      constexpr operator U&( this Self&& self )
          SL_expr_equiv_no_ret( details::forward_like< Self >( self.value ) )

      template < class... Ts, class Self >
      SL_using_m operator( )( [[maybe_unused]] this Self&& self, [[maybe_unused]] Ts&&... args )
          SL_expr_equiv( details::forward_like< Self >( self.value ) )
    };

    template < details::satisfy< std::is_default_constructible > T, std::size_t id = 0 >
    inline storage_t< T > storage{ };

    template < auto value, std::size_t id = 0 >
    SL_using_m constant = storage_t< std::remove_cvref_t< decltype( value ) > const >{ value };

    template < class U, std::size_t id = 0 >
    SL_using_m $_ = coprojector_t< U >{ }( storage< U, id > );

    template < auto value, std::size_t id = 0 >
    SL_using_m $c
        = coprojector_t< std::remove_reference_t< decltype( value ) > >{ }( constant< value, id > );

  } // namespace factory

  inline namespace hkt {
    template < template < class > class > struct fmap_t; // Functor
    template < template < class > class > struct pure_t; // Applicative
    template < template < class > class > struct bind_t; // Monad

    template <>
    struct fmap_t< lambda > { // fmap<lambda> :: (a ... -> b) -> lambda<a> ... -> lambda<b>
      /// @note: This operator() need to be specially handled since if we just copy and paste the
      ///        lambda body (replace captures by `declval<>()`) it will trigger ICE of both clang
      ///        and gcc. Possibly due to we try to do some complicate lambda capture in a recursive
      ///        context.
      template < class Func >
      SL_using_c operator( )( Func&& func ) SL_expr_equiv_declval(
          ( auto{ std::forward< Func >( func ) } ), // only handle copy/move construct capture here,
                                                    // lambda body is not in immediate context
                                                    // and since it's a dependent lambda, it's safe
                                                    // to delay checks
          [func{ std::forward< Func >(
              func ) }]< class Self, details::satisfy< is_short_lambda >... Ts >(
              [[maybe_unused]] this Self& self0,
              [[maybe_unused]] Ts&&... args0 )
          // FIXME: this spec seems wrong since we also need to
          // check whether we could constructor lambda<(lambda)>
          SL_expr_equiv_declval(
              /// NOTE: again, we only check decay copy here.
              ( (void) auto{ details::forward_like< Self >( std::declval< Func&& >( ) ) },
                ( (void) auto{ std::declval< Ts&& >( ) }, ... ) ),
              ::short_lambda::lambda {
                [
                  func{ details::forward_like< Self >( func ) },
                  ... args0{ std::forward< Ts >( args0 ) }
                ]< class Self1, class... Ts1 >( [[maybe_unused]] this Self1&& self1, Ts1&&... args1 )
                    SL_expr_equiv_declval(
                        ( details::forward_like< Self1 >(
                            details::forward_like< Self >( std::declval< Func&& >( ) ) )(
                            details::forward_like< Self1 >(
                                std::forward< Ts >( std::declval< Ts&& >( ) ) )(
                                std::forward< Ts1 >( std::declval< Ts1&& >( ) )... )... ) ),
                        details::forward_like< Self1 >( func )( details::forward_like< Self1 >(
                            args0 )( std::forward< Ts1 >( args1 )... )... ) )
              } ) )
    };
    template <> struct pure_t< lambda >: lift_t {
      // pure<lambda> :: a -> lambda<a>
    };
    template <> struct bind_t< lambda > {
      // bind<lambda> :: lambda<a>... -> (a... -> lambda<b>) -> lambda<b>
      template < details::satisfy< is_short_lambda >... Ts1 >
      SL_using_c operator( )( Ts1&&... as )
          SL_expr_equiv_spec( ( (void) auto{ std::declval< Ts1&& >( ) }, ... ) ) {
        return
            [... as{ std::forward< Ts1 >(
                as ) } ]< class Self, class Func >( [[maybe_unused]] this Self&& self, Func&& func )
                SL_expr_equiv_spec(
                    // FIXME: this spec seems wrong since we also need to
                    // check whether we could constructor lambda<(lambda)>
                    (void) auto{ details::forward_like< Self >( std::declval< Func >( ) ) },
                    ( (void) auto{ details::forward_like< Self >( std::declval< Ts1&& >( ) ) },
                      ... ) ) {
                  return lambda {
                    [
                      func{ details::forward_like< Self >( func ) },
                      ... as{ details::forward_like< Self >( as ) }
                    ]< class Self1, class... Ts >
                      requires ( details::satisfy<
                                 decltype( details::forward_like< Self1 >(
                                     details::forward_like< Self >( std::declval< Func && >( ) ) )(
                                     details::forward_like< Self1 >( std::declval< Ts1 && >( ) )(
                                         std::forward< Ts >( std::declval< Ts && >( ) )... )... ) ),
                                 is_short_lambda > )
                    ( [[maybe_unused]] this Self1&& self1, Ts&&... args ) SL_expr_equiv(
                        details::forward_like< Self1 >( func )( details::forward_like< Self1 >( as )(
                            std::forward< Ts >( args )... )... )( std::forward< Ts >( args )... ) )
                  };
                };
      }
    };


    template < template < class > class Func > SL_using_m fmap = fmap_t< Func >{ };
    template < template < class > class Func > SL_using_m pure = pure_t< Func >{ };
    template < template < class > class Func > SL_using_m bind = bind_t< Func >{ };
  } // namespace hkt

} // namespace short_lambda
