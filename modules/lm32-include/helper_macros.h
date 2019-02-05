/*!
 *
 * @brief     Some helpful macro definitions
 *
 * @file      helper_macros.h
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author    Ulrich Becker <u.becker@gsi.de>
 * @date      30.10.2018
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#ifndef _HELPER_MACROS_H
#define _HELPER_MACROS_H

#include <stddef.h> // Necessary for the macro "offsetof()"
#include <limits.h> // Necessary for constant "CHAR_BIT" (in the most cases always 8)

#if defined(__SSP__) || defined(__SSP_ALL__) || defined(__SSP_STRONG__) || \
    defined(__DOXYGEN__)
/*!
 * @brief This macro becomes defined when the compiler is invoked with option
 *        -fstack-protector or -fstack-protector-all
 *
 * That means, extra code for the stack-protector will produced for the
 * so called "Stack Smashing Protector" (SSP)
 */
 #define CONFIG_STACK_PROTECTOR_CODE

#endif /* if defined(__SSP__) || defined(__SSP_ALL__) */

#if defined(__GNUC__) || defined(__DOXYGEN__)
/*!
 * @brief Macro represents the full version number of the compiler as integer
 *        value.
 *
 * E.g.: If the compilers version - displayed by the command line option
 *       gcc --version - 7.3.0 then this macro will generate the number 70300.
 */

 #define COMPILER_VERSION_NUMBER (__GNUC__ * 10000 \
                                + __GNUC_MINOR__ * 100 \
                                + __GNUC_PATCHLEVEL__)
/*!
 * @brief Macro expands to a zero terminated ASCII string of the compiler-version.
 *
 * E.g.: If the compilers version - displayed by the command line option
 *       gcc --version - 7.3.0 then this macro will substituted by: "7.3.0"
 */
 #define COMPILER_VERSION_STRING  TO_STRING(__GNUC__)"." \
                                  TO_STRING(__GNUC_MINOR__)"." \
                                  TO_STRING(__GNUC_PATCHLEVEL__)
#else
 #define COMPILER_VERSION 0
 #define COMPILER_VERSION_STRING "unknown"
 #warning "Unknown compiler therefore its not possible to determine the macro COMPILER_VERSION !"
#endif

/*!
 * @brief Macro will be substituted by the number of elements of the given array.
 * @param a Name of the c-array variable
 * @return Number of array-elements
 * 
 * Example:
 * @code
 * int myArray[42], i;
 * for( i = 0; i < ARRAY_SIZE(myArray); i++)
 *    myArray[i] = i;
 * @endcode
 */
#ifndef ARRAY_SIZE
 #define ARRAY_SIZE( a ) ( sizeof(a) / sizeof((a)[0]) )
#endif

/*!
 * @brief Returns the size in bits of the given data type,
 * in contrast to sizeof() which returns the size in bytes.
 *
 * @param TYP data type
 * @return Number of bits for the given data type.
 */
#define BIT_SIZEOF( TYP ) (sizeof(TYP) * CHAR_BIT)


#ifndef __GNUC__
  #warning "Compiler isn't a GNU- compiler! Therefore it's not guaranteed that the following macro-definition PACKED_SIZE will work."
#endif
#ifdef PACKED_SIZE
  #undef PACKED_SIZE
#endif
/*!
 * @brief Modifier- macro forces the compiler to arrange the elements of a \n
 *        structure in the smallest possible size of the structure.
 * @see STATIC_ASSERT
 * @note At the moment this macro has been tested for GCC- compiler only!
 */
#define PACKED_SIZE __attribute__((packed))

/*!
 * @brief Modifier- macro for variables which are not used.
 *
 * In this case the compiler will suppress a appropriate warning. \n
 * This macro is meaningful for some call-back functions respectively
 * pointers to them, with a unique policy of parameters. For example
 * interrupt service routines:
 * @code
 * void interruptHandler( int interruptNumber UNUSED, void* pContext UNUSED )
 * {
 *  // No warning when "interruptNumber" and/or "pContext" will not used.
 * }
 * @endcode
 * @note At the moment this macro has been tested for GCC- compiler only!
 */
#ifdef UNUSED
 #undef UNUSED
#endif
#define UNUSED __attribute__((unused))

/*!
 * @brief Generates a deprecated warning during compiling
 *
 * The deprecated modifier- macro enables the declaration of a deprecated
 * variable or function without any warnings or errors being issued by the
 * compiler. \n
 * However, any access to a deprecated variable or function creates a warning
 * but still compiles.
 */
#ifdef DEPRECATED
 #undef DEPRECATED
#endif
#define DEPRECATED __attribute__((deprecated))

/*!
 * @brief Declares a function as always inline.
 *
 * Generally, functions are not inlined unless optimization is specified. \n
 * For functions declared inline, this attribute inlines the function independent
 * of any restrictions that otherwise apply to inlining. \n
 * Failure to inline such a function is diagnosed as an error. \n
 * @Note that if such a function is called indirectly the compiler
 * may or may not inline it depending on optimization level and a failure
 * to inline an indirect call may or may not be diagnosed.
 */
#define ALWAYS_INLINE __attribute__((always_inline))

#ifndef STATIC_ASSERT
#if ((__cplusplus > 199711L) || (COMPILER_VERSION_NUMBER >= 40600))
  #ifndef __cplusplus
     #define static_assert _Static_assert
  #endif
  #define STATIC_ASSERT( condition ) static_assert( condition, "C-Macro: STATIC_ASSERT" )
#else
 #ifndef __DOXYGEN__
  #define __STATIC_ASSERT__( condition, line ) \
       extern char static_assertion_on_line_##line[2*((condition)!=0)-1];
 #endif
/*!
 * @brief Macro produces a compiletime-error if the given static condition
 *        isn't true.
 * @param condition static condition to test
 *
 * Example 1 should be ok.
 * @code
 * typedef struct
 * {
 *    char x;
 *    int  y;
 * } PACKED_SIZE FOO;
 * STATIC_ASSERT( sizeof(FOO) == (sizeof(char) + sizeof(int))); // OK
 * @endcode
 *
 * Example 2 should make a compiletime-error.
 * @code
 * typedef struct
 * {
 *    char x;
 *    int  y;
 * } BAR;
 * STATIC_ASSERT( sizeof(BAR) == (sizeof(char) + sizeof(int))); // Error
 * @endcode
 * @see PACKED_SIZE
 */
  #define STATIC_ASSERT( condition ) __STATIC_ASSERT__( condition, __LINE__)
#endif // if (__cplusplus >  199711L) && !defined(__lm32__)
#endif // ifndef STATIC_ASSERT

/*!
 * @brief Cast a member of a structure out to the containing structure.
 *
 * This macro has been adopt from the Linux kernel-source.
 * Origin in <kernel-source>/include/linux/kernel.h as "container_of".
 *
 * This macro has also been tested successful with the compiler "lm32-elf-gcc"
 * version 4.5.3 and 7.3.0, and in C++ environment as well.
 *
 * @note The condition for working this macro properly is, that the content
 *       structures of the container structure has to be flat members,
 *       that means not as pointers (or in C++ as reverences). \n
 *       <b>Otherwise this macro will crash!</b>
 *
 * @note Keep in mind: This macro is and remains a hack, be careful
 *       and use it only if you know exactly what you are doing!
 *
 * @param ptr    The pointer to the member.
 * @param type   The type of the container struct this is embedded in.
 * @param member The name of the member within the container struct.
 * @return The pointer of the container-object including this member.
 */
#define CONTAINER_OF(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})

/*!
 * @see CONTAINER_OF
 */
#define CONTAINER_OF_ARRAY(ptr, type, mArray, index) ({            \
        const typeof( ((type *)0)->mArray[0] ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - (offsetof(type, mArray[0]) +    \
        (index) * sizeof(mArray[0])));})


#ifdef TO_STRING_LITERAL
   #undef TO_STRING_LITERAL
#endif
#ifdef TO_STRING
   #undef TO_STRING
#endif
/*!
 * @brief helper macro for TO_STRING.
 */
#define TO_STRING_LITERAL( s ) # s

/*!
 * @brief Converts a constant expression to a zero terminated ASCII string.
 *
 * E.g.: The expression:
 * @code
 * #define MY_NUMBER 42
 * const char* str = "My number is " TO_STRING( MY_NUMBER ) "\n";
 * @endcode
 * builds following string during the compile time:
 * @code
 * const char* str = "My number is 42\n";
 * @endcode
 * @param s Constant expression
 * @return Zero terminated ASCII string.
 */
#define TO_STRING( s ) TO_STRING_LITERAL( s )

/*!
 * @brief Will used from DECLARE_CONVERT_BYTE_ENDIAN
 *        and IMPLEMENT_CONVERT_BYTE_ENDIAN
 */
#define __FUNCTION_HEAD_CONVERT_BYTE_ENDIAN( TYP )      \
   TYP convertByteEndian_##TYP( const TYP value )

/*!
 * @brief Will used from macro IMPLEMENT_CONVERT_BYTE_ENDIAN
 *        and C++ template "convertByteEndian" in the
 *        case of C++ only.
 */
#define __FUNCTION_BODY_CONVERT_BYTE_ENDIAN( TYP )      \
   {                                                    \
      TYP result;                                       \
      int i;                                            \
      for( i = sizeof(TYP)-1; i >= 0; i-- )             \
        ((unsigned char*)&result)[i] =                  \
           ((unsigned char*)&value)[sizeof(TYP)-1-i];   \
      return result;                                    \
   }                                                    \

/*!
 * @brief Generates a prototypes of endian converting functions
 *        for header-files.
 * @see IMPLEMENT_CONVERT_BYTE_ENDIAN
 * @param TYP Integer type of value to convert.
 */
#define DECLARE_CONVERT_BYTE_ENDIAN( TYP ) \
   __FUNCTION_HEAD_CONVERT_BYTE_ENDIAN( TYP );

/*!
 * @brief Generates functions converting little to big endian
 *        and vice versa.
 *
 * @see DECLARE_CONVERT_BYTE_ENDIAN
 * Example:
 * @code
 * IMPLEMENT_CONVERT_BYTE_ENDIAN( uint64_t )
 * @endcode
 * implements a function with the following prototype:
 * @code
 * uint64_t convertByteEndian_uint64_t( const uint64_t )
 * @endcode
 * @see DECLARE_CONVERT_BYTE_ENDIAN
 * @param TYP Integer type of value to convert.
 *
 * ... Unfortunately in contrast to C++ C doesn't understand templates. :-/
 */
#define IMPLEMENT_CONVERT_BYTE_ENDIAN( TYP )            \
   __FUNCTION_HEAD_CONVERT_BYTE_ENDIAN( TYP )           \
   __FUNCTION_BODY_CONVERT_BYTE_ENDIAN( TYP )

#if defined(__cplusplus ) || defined(__DOXYGEN__)
/*!
 * @brief Template converts the given value from little to big endian
 *        and vice versa.
 * @note For C++ only!
 * @param value Integer value to convert.
 * @return Converted value.
 */
template <typename TYP> TYP convertByteEndian( const TYP value )
   __FUNCTION_BODY_CONVERT_BYTE_ENDIAN( TYP )
#endif /* __cplusplus */

#ifndef __cplusplus

#ifndef min
   /*!
    * @brief Returns the smaller value of the parameters "a" or "b"
    */
   #define min( a, b ) \
   ({ typeof(a) _a = (a); \
      typeof(b) _b = (b); \
   (_a < _b)? _a : _b; })
#endif

#ifndef max
   /*!
    * @brief Returns the greater value of the parameters "a" or "b"
    */
   #define max( a, b ) \
   ({ typeof(a) _a = (a); \
      typeof(b) _b = (b); \
   (_a > _b)? _a : _b; })
#endif

#endif /* ifndef __cplusplus */

#endif // ifndef _HELPER_MACROS_H
//================================== EOF ======================================