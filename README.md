Φ - The Phi Programming Language
======================================

Phi is a high-level, functional programming language, that combines the clean and stable nature of Haskell with the simplicity and strength of C. It is meant to reduce the learning barrier between procedural and functional languages. The syntax was heavily inspired by [Haskell](https://www.haskell.org) and [F#](https://fsharp.org), however it uses Revere Polish Notation to imitate the flow of data through the code (input -> function -> output -> next function).

An important feature of Phi, that sets it apart from almost all languages currently available, is the possibility to return more than one value. In current languages like Python, Haskell or F#, the only way to return more than one thing is through the use of Tuples. Phi supports this natively. An example of this can be found below.

Sample Code
-----------

```
# Declare a function that takes to inputs and returns two outputs
new Number:x Number:y -> divrem -> Number:q -> Number:rem
	x y div toInt q
	q x-y*q return
```
