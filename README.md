# Φ - The Phi Programming Language

Phi is a high-level, functional programming language, that combines the clean and stable nature of Haskell with the simplicity and strength of C. It is meant to reduce the learning barrier between procedural and functional languages. The syntax was heavily inspired by [Haskell](https://www.haskell.org) and [F#](https://fsharp.org), however it uses Reverse Polish Notation to imitate the flow of data through the code (input -> function -> output -> next function).

An important feature of Phi, that sets it apart from almost all languages currently available, is the possibility to return more than one value. In current languages like Python, Haskell or F#, the only way to return more than one thing is through the use of Tuples. Phi supports this natively. An example of this can be found below.

## Sample Code

```
/* Compute the square root of a using the Babylonian Method and return square root and error */
new Real:a -> babylon -> Real:sqrt Real:err
	a abs a :
	1 store sqrt :
	sqrt-a abs err :
	while err > 0.00001 (
		(sqrt + a/sqrt)/2 store sqrt :
		sqrt-a/sqrt abs err
	) end :
	sqrt err
```

## Basic Syntax

A Phi-command consists of a sequence of Identifiers or Constant Literals. As mentioned above, Phi uses Reverse Polish Notation, which means that a function call is performed by listing out the arguments, followed by the function name. The output of a function call can then either be stored in a variable, or used as input to another function. Commands can be chained by placing a colon (:) between them.

### Variables

Variables in Phi are categorized into two distinct groups. The first group is said to *provide* data, and consists of those variables that are used as input for some function call or function return value. The second group is said to *absorb* data and consists of variables that are used to store the result of a function call or literal.

Clearly, literals are always providing data, as their value cannot be changed. Whether a variable is providing or absorbing data, depends on its position in the surrounding command. By default, any variable occurring before a function call is providing data, any variable occurring after a function call is absorbing data. You can override the latter by appending ":v" to the variable name to declare it as providing data. Example:

```
	a function1 b		// stores the result of function1 into the variable b
	a function1 b:v		// Provides the result of function1 and the value of b for the next function call
```

For a simple variable assignment without calling into a function, use the keyword store. It is used in Code just like a function call, but is evaluated at compile time. In effect, *store* is a type-independent identity-function that returns its input unaltered. If store and *:v* are used in conjunction, then *:v* supersedes.

New variables are created by appending ":!" to the variable name. New variables are always absorbing data, and this behaviour cannot be altered. Notice that there is no need to specify a type for a new variable, since it is inferred automatically from the value stored into it. To explicitly mention a type, either for debugging or maintainability, you may use comments at any point in the code (except in the middle of an identifier or literal).

An important feature of Phi's ability to return multiple values is the fact that data can be only partially absorbed. More specifically, If a function returns, say, three values, then the first one can be stored into a variable while the second and third are passed to another function. This allows for a very rich structure with simple syntax, but it also hides the danger of writing horribly unreadable code.

### Functions

A function is declared using the *new* keyword, followed by the function's prototype, followed by a sequence of commands (chained with ':'). A Prototype defines the input arguments to the function, the functions name and its return values (in this order).

The input arguments are simply a list of type-name pairs in the form *Type:Name*, where *Type* is either Real or Bool (More types are coming) and *Name* is any valid identifier. If another identifier with this name already exists, it will be shadowed for the duration of the function call, but it becomes available again once the function ends.

The list of input arguments ends with an arrow (->), followed by an identifier used as the function name, another arrow and the output parameters. The latter are provided in the same way as the input arguments, with the exception that the name can be omitted, so a simple type name suffices. If an identifier is provided, then a local variable with that name will be created, however there is no association between the name and the output parameter - it is simply a syntactic simplification!

The input list can be empty, in which case the first arrow i omitted as well, but it must return at least one value! Example:
```
Real:x -> func1 -> Real		// valid
func1 -> Real:x			// valid
Real:x -> func1			// invalid!
```

The return value of the function is automatically created from the data available at the end of the last command. In general, this corresponds to listing out the values to return (in the appropriate order) in the last line of the function body, but in some cases this can also be the return value of another function as well.

### Control Flow

Unlike other functional languages, Phi provides basic Control Flow functionality, namely a while-loop and a conditional block (if-else). Their syntax is fairly similar, so it will only be explained for the loop, but can be translated to the conditional block by replacing all "while" by "if".

A loop starts with the keyword *while* followed by an expression, which is used as the loop condition. After that comes the loop body, which consists of a single command or a sequence of commands enclosed in simple parentheses ('(' and ')'). The loop body ends with the keyword *end*. Optionally, instead of *end*, the loop can be followed by *else* and another command. This piece of code will be executed, if the loop body did not execute a single time. The *else* block need not be ended with *end* (and doing so will create a syntax error). Once again, to chain commands in this block, you must use parentheses.

*Note:* Variables use Block-scope. This means that any variable created in the loop-body is only available within the loop body, the same goes for the else-block. If you want to use a variable both inside and out of the loop, initialize it beforehand!

## Some Notes on Vectors, AVX and Optimization

You can easily create Vectors in Phi by appending the length of the vector in pointy brackets to the type name, e.g. Real<4> for a vector of type Real and length 4. The interface for vectors is exactly the same as that for primitive types, making working with them particularly easy.

However, this does **not** mean, that the code does indeed run with vectors of the given size. LLVM does its best job to produce proper vector instructions for the native Machine, but not all vectors sizes work equally well. Consequently, the IR code produced by LLVM may only use vectors of size two, making the resulting code slower than necessary.

Solving this is a task on LLVM's coding team, not me. Still, there is an easy work around, which might boil down to a no-op, if you were aiming for the ultimate fastest code already anyway.

Phi cannot currently produce executables directly. Instead, it produces an object file, which must then be linked used any common compiler like GCC or clang. Both of these provide optimizations already, one of which involves vectorization. Thus, to get the best vectorized experience possible, the workflow is as follows:
 * Use vector types in Phi
 * Produce a (potentially less vectorized) object file
 * Compile with GCC/Clang with optimizations on to produce a highly vectorized executable
In fact, using Vector Types in Phi is not strictly necessary, but doing so helps the optimizer recognize, where vector instructions are used/useful.

## Interfacing with other languages

Phi produces an object file. This object file can be linked against any other object file on the same target machine (i.e. the native host) by using any compiler/linker like Clang/GCC or the like. To declare a function that is defined elsewhere, use the *extern* keyword followed by the functions prototype. In this case, all parameter names can be omitted - and if any are provided, they are simply ignored. The type correspondences between Phi and C are given below.
 * Real -> double
 * Bool -> int
To make use of the multiple return values of a Phi function within a different language (say a C program calls a Phi function), you may be able to define a struct that contains the same fields in the same order. However there is no guarantee this will work in all cases.

## The Binary Operators

Phi defines the usual plethora of binary operators, however there are some important notes to be mentioned here. The list below shows the type constellation of the operation on the left and the corrensponding C-code on the right.
 * Bool:b1 \* Bool:b2 == b1 && b2
 * Bool:b1 \* Real:x1 == b1 ? x1 : 0
 * Real:x1 \* Bool:b2 == b2 ? x1 : 0
 * Bool:b1 \+ Bool:b2 == b1 ^ b2
If both operands are of vector type, then the operations are performed elementwise. No scalar-vector mutiplication is supported as of now. The only exception are scalar booleans, in which case the entire vector is either kept or null'ed - depending on the truth value.
