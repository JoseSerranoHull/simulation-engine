# Complete C++ Programming and Design Lab Book (A - H)
---
## MSc Computer Science for Games Programming 
---

**Course: 700120 - C++ Programming and Design**
**Name: JOSE JAVIER SERRANO SOLIS**

September 2025 – January 2026 

Warren Viant

---

## Lab A
### Week 1 - Lab A

03 Oct 2025

#### Q1. Copilot tutorial

**Question:**

Complete the short Copilot tutorial on the Microsoft Learn website, to gain an understanding of how to use Copilot for more than just writing code.

https://learn.microsoft.com/en-us/visualstudio/debugger/debug-with-copilot?view=vs-2022

Note:
- This tutorial uses C#, but it does not require any previous knowledge of C#
- The answers provided by Copilot can vary each time you use Copilot.

**Solution:**

```c#
public class Example
{
    public static void Main(string[] args)
    {
        int value = Int32.Parse(args[0]);
        List<String> names = null;
        if (value > 0)
            names = new List<String>();

        // names.Add("Major Major Major");
        foreach (var item in args)
        {
            names.Add("Name: " + item);
        }
    }
}
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt the multiple methods that Copilot can help you:
- Ask him directly in the Copilot window.
- Right click on a line of code with or without breakpoints.
- Suggesting in the Conditional Breakpoints if statements.
- While not an method of communication, you can insert the code that suggests to you.

*Did you make any mistakes?* 
I couldn't get the find the Debug > General > Open debug launch profiles UI at the last segment of the tutorial, while pressing right click on the solution, it did appeard the Debug options on the window but not the general one.

*In what way has your knowledge improved?*
In that Copiot is a powerful tool, that can appear and help me in diverse ways within Visual Studio.

**Questions:**

*Is there anything you would like to ask?* Is the tutorial from Windows not up to date, because I find it wierd that even although I searched online for answers or played around in Visual Studio I couldn't find the "Open debug launch profiles UI" option.

#### Q2. Hello World

**Question:**

Locate the Solution Explorer within Visual Studio and select the HelloWorld project. Right click on this project and select Build. This should compile and link the project. Now run the HelloWorld program.

Change between Debug and Release mode. Compile again and rerun the program.

Note: It is good practice when transferring projects between different PCs to "retarget" the project to the latest version of the compiler. This is done by right clicking on the project (or solution) in the Solution Explorer and selecting "Retarget". Remember to "Rebuild" the project after a change of target.

**Solution:**

```c++
#include <iostream>

int main(int, char**) {
	std::cout << "Hello World" << std::endl;
	return 0;
}
```

**Sample output:**

With Debug mode:
```
Hello World

C:\Users\949145\GitHub\lab-a-JoseSerranoHull\x64\Debug\Hello World.exe (process 16692) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .
```

With Release mode:
```
Hello World

C:\Users\949145\GitHub\lab-a-JoseSerranoHull\x64\Release\Hello World.exe (process 10672) exited with code 0 (0x0).
Press any key to close this window . . .
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned to start a project on Visual Studio. To build directly by clicking the **.sln** on the *Solution Explorer* window and that is a good practice to do so when opening a project on a new machine.

*Did you make any mistakes?* 
I couldn't where to change from Debug to Release at the start, while plaing around I found how to do it.

*In what way has your knowledge improved?*
I understand the differences between a Debug run and a Release run, and good practices of building a project before working on them in a nwe machine.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q3. Console Window

**Question:**

A command window is automatically opened when you run a console application. This window is also automatically closed when the program terminates. Delay the termination of the program by adding the following two lines to the end of your code:

```c++
    int keypress;
    std::cin >> keypress;
```

This now requests an integer value before termination of the program

**Solution:**

```c++
#include <iostream>

int main(int, char**) {
	std::cout << "Hello World" << std::endl;
    int keypress;
    std::cin >> keypress;
	return 0;
}
```

**Test data:**

- An int e.g. 1
- A char e.g. a

**Sample output:**

- An int e.g. 1
```
Hello World
1

C:\Users\949145\GitHub\lab-a-JoseSerranoHull\x64\Release\Hello World.exe (process 5656) exited with code 0 (0x0).
Press any key to close this window . . .
```

- A char e.g. a
```
Hello World
a

C:\Users\949145\GitHub\lab-a-JoseSerranoHull\x64\Release\Hello World.exe (process 18616) exited with code 0 (0x0).
Press any key to close this window . . .
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned about cin and its qualities. Along with how to stop a programming from closing, like a simple break.

*Did you make any mistakes?*
No as far as I know.

*In what way has your knowledge improved?*
Becasuse of my following question I researched a bit about cin, and how does it accepts input from the standard input stream.

**Questions:**

*Is there anything you would like to ask?*
Why does it also accept char as values no the user input, shoudn't only accept int values? According to a research I did cin is an object of istream class that is used to accept the input from the standard input stream so Does it also applies to int? 

#### Q4. Includes

**Question:**

Remove the statement:

```c++
    #include <iostream>
```

Compile the program. What is the effect? Replace the statement and continue.

Notice that we do not add the .h extension to the header file name. Extensions are only added for your own header files or for legacy system header files.

**Solution:**

```c++
int main(int, char**) {
	std::cout << "Hello World" << std::endl;
    int keypress;
    std::cin >> keypress;
	return 0;
}
```

**Sample output:**

Error, couldn't run the program.
```
Build started at 15:13...
1>------ Build started: Project: Hello World, Configuration: Release x64 ------
1>Source.cpp
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(2,7): error C2039: 'cout': is not a member of 'std'
1>    C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\predefined C++ types (compiler internal)(357,11):
1>    see declaration of 'std'
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(2,7): error C2065: 'cout': undeclared identifier
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(2,37): error C2039: 'endl': is not a member of 'std'
1>    C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\predefined C++ types (compiler internal)(357,11):
1>    see declaration of 'std'
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(2,37): error C2065: 'endl': undeclared identifier
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(4,7): error C2039: 'cin': is not a member of 'std'
1>    C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\predefined C++ types (compiler internal)(357,11):
1>    see declaration of 'std'
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(4,7): error C2065: 'cin': undeclared identifier
1>Done building project "Hello World.vcxproj" -- FAILED.
========== Build: 0 succeeded, 1 failed, 0 up-to-date, 0 skipped ==========
========== Build completed at 15:13 and took 00.473 seconds ==========

```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned about the include in C++ and how they are necessary in order for them to run the program, of course, if you are using them in your code but never calling them is boudn to have errors.

*Did you make any mistakes?*
No as far as I know.

*In what way has your knowledge improved?*
As in seeing an example of a bad practice and what would happen.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q5. Namespace

**Question:**

Add the statement

```c++
using namespace std;
```

Compile the program. What is the effect? Now remove all instances of the code

```c++
std::
```

Compile the program. What is the effect? Now remove

```c++
using namespace std;
```

The statement informs the C++ compiler to look in the std namespace for any names/labels that it cannot find in the programs default namespace.

**Solution:**

```c++
#include <iostream>
using namespace std;

int main(int, char**) {
    int keypress;
	return 0;
}
```

Then...

```c++
#include <iostream>

int main(int, char**) {
    int keypress;
	return 0;
}
```

**Sample output:**

The program run without any issues, although I thought there where goin to have problems.
```
Build started at 15:15...
1>------ Build started: Project: Hello World, Configuration: Release x64 ------
1>Source.cpp
1>C:\Users\949145\GitHub\lab-a-JoseSerranoHull\Hello World\Source.cpp(5,6): warning C4101: 'keypress': unreferenced local variable
1>Generating code
1>Previous IPDB and IOBJ mismatch, fall back to full compilation.
1>All 1 functions were compiled because no usable IPDB/IOBJ from previous compilation was found.
1>Finished generating code
1>Hello World.vcxproj -> C:\Users\949145\GitHub\lab-a-JoseSerranoHull\x64\Release\Hello World.exe
1>Done building project "Hello World.vcxproj".
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
========== Build completed at 15:15 and took 01.122 seconds ==========
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned about the use of namespaces, and how they are link, sort of, for code that request fucntions or values from them to work.

*Did you make any mistakes?*
No as far as I know.

*In what way has your knowledge improved?*
As in seeing an example of a use case of namespaces.

**Questions:**

*Is there anything you would like to ask?*
Yes, If I deleted the #include then it doesn't work because it can't find the references to cin, cout and endl, but removing the namespace apparently does nothing?

#### Q6. Create a new project

**Question:**

Create a new Visual C++ Empty project called “Temperature” by using the New Project dialog File->New->Project. You should now have a new project which contains no files.

**Solution:**

```c++
// Temperature.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

int main()
{
    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
```

**Sample output:**
```
Hello World!
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned about the different templates that are already available on Visual Studio, and how easy it is to pick a wrong one.

*Did you make any mistakes?*
Yes, I pick a template for a Windows project in C++ instead of a simple C++ console project.

*In what way has your knowledge improved?*
That is easy to get confused by which template to pick.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q7. Temparature

**Question:**

Create a new cpp file within the temperature project and write a program to input a Fahrenheit measurement, convert it and output a Celsius value. The conversion formula is

```c++
c = 5/9 (f - 32)
```

Confirm that your conversion programme gives the correct outputs

- 32 F gives 0 C
- 33 F gives 0.555 C

**Solution:**

The Converter class body:
```c++
#include "FahrenheitConverter.h"
#include <iostream>

float FahrenheitConverter::toCelsius(float fahrenheit) {
    return (fahrenheit - 32) * 5.0f / 9.0f;
}
```

The Converter class header:
```c++
class FahrenheitConverter { 
public:
    float toCelsius(float f);
};
```

The Temperature.cpp file:
```c++
#include <iostream>
#include "FahrenheitConverter.h" // Make sure this header exists and is in your project

int main()
{
    float farenheit;
    std::cout << "Enter temperature in Farenheit: ";
    std::cin >> farenheit;

    FahrenheitConverter converter;
    float celsius = converter.toCelsius(farenheit);
    std::cout << farenheit << " F gives " << celsius << " C" << std::endl;

    return 0;
}
```

**Test data:**

- 32
- a
- 100

**Sample output:**

- 32 F gives 0 C
```
Enter temperature in Farenheit: 32
32 F gives 0 C

C:\Users\949145\source\repos\Temperature\x64\Debug\Temperature.exe (process 3640) exited with code 0 (0x0).
Press any key to close this window . . .
```
- 0 F gives -17.7778 C
```
Enter temperature in Farenheit: 0
0 F gives -17.7778 C

C:\Users\949145\source\repos\Temperature\x64\Debug\Temperature.exe (process 10808) exited with code 0 (0x0).
Press any key to close this window . . .
```
- 100 F gives 37.7778 C
```
Enter temperature in Farenheit: 100
100 F gives 37.7778 C

C:\Users\949145\source\repos\Temperature\x64\Debug\Temperature.exe (process 24880) exited with code 0 (0x0).
Press any key to close this window . . .

```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I reviewed my knowledge in headers, public funcitons, body and statments.

*Did you make any mistakes?*
Yes,  It's been a long time since I wrote C++ code, so I stopped myself a couple of times in order to write the code.

*In what way has your knowledge improved?*
I'm remembering how to write C++ code, how headers work, are included and how to call functions.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q8. Auto, const and casting

**Question:**

Now rewrite your temperature example using the auto keyword, constants and explicit casting

Does this make the code easier or more difficult to understand?

**Solution:**

The Converter class body:
```c++
#include "FahrenheitConverter.h"
#include <iostream>

float FahrenheitConverter::toCelsius(int fahrenheit) {
    const int value = 32;
    return ((float)fahrenheit - value) * 5.0f / 9.0f;
}
```

The Converter class header:
```c++
class FahrenheitConverter { 
public:
    float toCelsius(int f);
};
```

The Temperature.cpp file:
```c++
#include <iostream>
#include "FahrenheitConverter.h" // Make sure this header exists and is in your project

int main()
{
    float farenheit;
    std::cout << "Enter temperature in Farenheit: ";
    std::cin >> farenheit;

    FahrenheitConverter converter;
    auto celsius = converter.toCelsius(farenheit);
    std::cout << farenheit << " F gives " << celsius << " C" << std::endl;

    return 0;
}
```

**Test data:**

- 32

**Sample output:**

- 32 F gives 0 C
```
Enter temperature in Farenheit: 32
32 F gives 0 C

C:\Users\949145\source\repos\Temperature\x64\Debug\Temperature.exe (process 3640) exited with code 0 (0x0).
Press any key to close this window . . .
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Checked how to use the keywords of const and auto, and also did a little explicit casting on some int value. Const is a powerful tool in order to let other people know what to move or to give name to a magic number. Auto can help out a lot if you don't need long type names or just more fast coding, but it's case by case. And Explicit casting as a way of easily convert types.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In how to properly use const, auto and explicit casting in C++.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q. Static Assert

**Question:**

Create a new project call sizeOf that includes the following lines of code:

```c++
const int sizeOfInt = sizeof(int);
const int sizeOfPointer = sizeof(int*);
static_assert (sizeOfInt == sizeOfPointer, "Pointers and int are different sizes");
```

Select a different architecture (e.g. x86 or x64) to see if you can make the assert fail.

Experiment by adding further asserts to your program

Remember these static asserts are completely free. The check is done at compile time, so no code is added to your solution.

**Solution:**

Create a new project call sizeOf that includes the following lines of code:

```c++
 const int sizeOfInt = sizeof(int);
 const int sizeOfPointer = sizeof(int*);
 static_assert (sizeOfInt == sizeOfPointer, "Pointers and int are different sizes");
```

But it gives me an error becasue the static_assert fails because sizeof(int) is not equal to sizeof(int*) on most platforms. Copilot says that in order to fix the error, update the assertion to check that they are NOT equal.

Thus

```c++
#include <iostream>

int main()
{
    const int sizeOfInt = sizeof(int);
    const int sizeOfPointer = sizeof(int*);
    static_assert(sizeOfInt != sizeOfPointer, "Pointers and int are the same size");
}
```

**Test data:**

- x64
- x86

**Sample output:**

- Success
```

```

- Failed
```
Build started at 15:20...
1>------ Build started: Project: sizeOf, Configuration: Debug Win32 ------
1>sizeOf.cpp
1>C:\Users\949145\source\repos\sizeOf\sizeOf\sizeOf.cpp(10,29): error C2338: static_assert failed: 'Pointers and int are the same size'
1>Done building project "sizeOf.vcxproj" -- FAILED.
========== Build: 0 succeeded, 1 failed, 0 up-to-date, 0 skipped ==========
========== Build completed at 15:20 and took 01.337 seconds ==========
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
According to a research I did:
Why does it fail on x86 but work on x64?
1. x86 (32-bit) Architecture
    - int is typically 4 bytes.
    - int* (pointer) is also 4 bytes.
    - So, sizeOfInt == sizeOfPointer (both are 4).
    - The static_assert fails, because the condition is false.
2. x64 (64-bit) Architecture
    - int is still 4 bytes.
    - int* (pointer) is 8 bytes.
    - So, sizeOfInt != sizeOfPointer (4 != 8).
    - The static_assert passes, because the condition is true.
---
Why are pointer sizes different?
    - Pointer size depends on the architecture's address space:
    - 32-bit (x86): 4 bytes (can address 2³² bytes of memory)
    - 64-bit (x64): 8 bytes (can address 2⁶⁴ bytes of memory)
    - int size is usually fixed at 4 bytes, regardless of architecture.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
That depending on the architecture the results shall change because on how it uses the spaces in memory.

**Questions:**

*Is there anything you would like to ask?*
No.

### Final Reflection
After finishing all the exercises I get to relearn core concepts of C++ that have been dormant at the back of my head for a long time.

From starting a project or the differences of compiling for x64 and x86, the advantages of the language are fundamentally on memory use and optimization among other things. Relearning the core helps me to get the hang of C++. I actually liked the temparature exercise because it got me investigating and using new tools like copilot (along with its tutorial) in order to get a function to work.

Header and body, format and structure of the language are things that I know now that I need to practice more.

---

## Lab B
### Week 2 - Lab B

10 Oct 2025

#### Introduction
This lab introduces you to the timing application. You will make a lot of use of this small piece of code in future labs When designing timing experiments, think very carefully about what you are actually timing and also what either the compiler or the CPU could be doing to invalidate your results.

#### Q1. Timing

**Question:**

Locate the Solution Explorer within Visual Studio and select the Timing project. Right click on this project and select Build. This should compile and link the project. Now run the Timing program.
This application attempts to time a very small piece of code to CPU clock precision.  

The solution consists of an assembly file (masm.asm) and a cpp file (source.cpp).  Ignore the assembly file for now.  Source.cpp first requests that the OS run the application with maximum priority, then it measures the overhead in calling the timing functions, and finally it runs a test on a small piece of code, "the payload".

Timing accurately on a modern CPU running Windows OS is very difficult, but it is possible to get accurate figures if you run an extremely large number of experiments and look for a pattern in the results. By default this application runs 250,000 iterations per experiment.

The experiment is currently set to measure the duration of the following piece of code:

```c++
for (auto j=0; j<20; j++) {
   dummyX = dummyX * 1.00001;
}
```

This will take approximate 70 CPU cycles on an Intel i7.

Run the application on your PC and take a look at the output.

- Overhead - time taken to call the timing functions
- Median - The middle time, when the list of times are sorted
- Mean - The mean of the times, discounting the lower and upper 10% of measurements

Try increasing the limit of the loop from 20 to 40.  Can you explain the result?

Now run the application both in Release x86 and Release x64 modes

Remember that it is generally pointless to time code in Debug mode.

**Solution:**
With 20 as a loop limit:
```c++
    // Run the actual experiment
	std::vector<DWORD> experimentTimes;
	for (auto i = 0; i < numOfIterations; i++) {
		const auto startTime = c_ext_getCPUClock();

		// BEGIN payload
		for (auto j = 0; j < 20; j++) {
			dummyX = dummyX * 1.00001;
		}
		// END payload

		const auto stopTime = c_ext_getCPUClock();
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
		experimentTimes.push_back(duration > 0 ? duration : 1);
	};
```

With 40 as a loop limit:
```c++
    // Run the actual experiment
	std::vector<DWORD> experimentTimes;
	for (auto i = 0; i < numOfIterations; i++) {
		const auto startTime = c_ext_getCPUClock();

		// BEGIN payload
		for (auto j = 0; j < 40; j++) {
			dummyX = dummyX * 1.00001;
		}
		// END payload

		const auto stopTime = c_ext_getCPUClock();
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
		experimentTimes.push_back(duration > 0 ? duration : 1);
	};
```

**Sample output:**

- With x64
With 20 as a loop limit:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 60

Mean (80%) duration: 59.9862

Sample data [100]
........................
56 56 56 56 56 56 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
........................
90 90 90 90 90 90 92 92 92 94 94 94 94 94 94 94 94 94 96 98 98 98 100 100 100 100 102 102 102 102 104 104 104 104 104 104 106 106 106 106 106 108 110 110 110 112 112 114 118 118 120 120 120 120 124 126 128 128 130 130 130 130 132 138 138 164 166 166 172 176 180 180 186 188 192 198 204 208 208 212 214 216 218 220 240 248 278 290 294 296 348 348 354 356 358 6354 19916 21118 29232 35736
........................

x= 5.18341e+21 (dummy value - ignore)
```

With 40 as a loop limit:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 126

Mean (80%) duration: 126.772

Sample data [100]
........................
122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122
........................
168 170 170 172 172 172 174 176 176 176 176 176 176 178 180 180 182 182 182 182 182 182 184 184 184 184 186 186 186 186 186 186 188 190 190 190 192 192 192 194 196 198 202 202 204 232 234 244 248 250 250 254 256 256 256 258 260 260 264 272 274 274 276 276 278 280 282 282 294 300 312 318 322 352 402 406 410 412 414 420 424 428 428 438 488 532 718 1002 3796 4364 4668 5280 6266 6414 12684 19692 19926 22340 29112 30574
........................

x= 2.68677e+43 (dummy value - ignore)
```

- With x86
With 20 as a loop limit:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 66

Mean (80%) duration: 66.8597

Sample data [100]
........................
62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64
........................
116 118 118 120 120 122 122 122 124 124 124 124 126 126 126 126 128 128 130 130 130 132 132 132 132 134 134 134 136 136 136 138 138 140 140 140 140 142 142 160 162 174 180 184 186 188 190 192 192 194 196 198 200 202 202 202 204 204 206 208 218 222 222 222 222 224 224 224 224 226 234 246 250 250 252 252 268 268 296 302 306 352 354 354 356 356 356 356 358 358 362 366 372 1680 15952 16898 17944 19290 19824 32522
........................

x= 5.18341e+21 (dummy value - ignore)
```

With 40 as a loop limit:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 132

Mean (80%) duration: 132.711

Sample data [100]
........................
126 126 126 126 126 126 126 126 126 126 126 126 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128
........................
2568 2588 2598 2602 2608 2608 2622 2624 2634 2644 2654 2656 2674 2678 2680 2684 2686 2704 2722 2756 2760 2772 2772 2792 2802 2802 2810 2826 2826 2850 2860 2872 2878 2886 2892 2916 2946 2958 2964 2966 2966 2982 2984 2996 2998 3010 3016 3040 3090 3096 3126 3130 3136 3138 3176 3204 3216 3226 3276 3292 3300 3358 3416 3428 3446 3446 3484 3526 3562 3584 3720 3730 3762 3832 4128 4268 4294 4298 4332 4588 4630 4648 4666 4734 4776 4944 4970 5052 18182 18294 18572 19970 20322 21192 22560 22708 26602 28072 28148 31034
........................

x= 2.68677e+43 (dummy value - ignore)
```
**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt about a solution to test the timing of your code:
- About the results with x64:
    - - Overhead durations are the same.
    - - Median duration increased from 60 to 126.
    - - Mean (80%) duration increased from 59.9862 to 126.772.

- About the results with x86:
    - - Overhead durations are the same.
    - - Median duration increased from 66 to 132.
    - - Mean (80%) duration increased from 66.8597 to 132.711.

This results tell me that with the x86 architecture the code runs a bit slower, but the increase in time is proportional to the increase in the loop limit. This means that the CPU is handling the increased workload efficiently, maintaining a consistent performance ratio between the two architectures.


*Did you make any mistakes?* 
No as far as I know.

*In what way has your knowledge improved?*
Thanks to reading the code I got to understand better how to measure the time of a piece of code and the process step by step on how making one, and also the differences between x86 and x64 architectures. And in order to do timing test and get meaningful results you need to run the code in Release mode.

**Questions:**

*Is there anything you would like to ask?* No.

#### Q2. Timing own code

**Question:**

Replace the payload with some of your own code.

When adding code to the payload, try and write code that the compiler will not optimise away.
A trick is to calculate a dummy value that is later printed. See dummyX in the example payload

If you are comparing the execution times between two sections of code, ensure that they are doing exactly the same work, otherwise the results will be meaningless.

**Solution:**

Changed how dummyX is calculated:

```c++
// Run the actual experiment
	std::vector<DWORD> experimentTimes;
	for (auto i = 0; i < numOfIterations; i++) {
		const auto startTime = c_ext_getCPUClock();

		// BEGIN payload
		for (auto j = 0; j < 20; j++) {
			dummyX += (j % 13) * 1.0001;
		}
		// END payload

		const auto stopTime = c_ext_getCPUClock();
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
		experimentTimes.push_back(duration > 0 ? duration : 1);
	}
```

**Sample output:**

With x64:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 88

Mean (80%) duration: 88.0619

Sample data [100]
........................
82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 82 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84
........................
202 204 204 208 208 210 212 212 212 214 214 216 218 220 220 220 222 224 226 228 228 228 228 228 228 228 230 232 232 232 232 234 234 236 238 238 238 240 242 242 242 244 246 246 248 248 248 252 254 256 256 256 256 258 260 262 266 268 268 268 270 274 274 274 274 276 278 280 282 282 286 300 300 300 332 348 350 372 378 410 416 426 428 428 432 434 450 492 536 732 1278 1390 4994 14610 19424 19638 21872 26566 26970 33662
........................

x= 2.47525e+07 (dummy value - ignore)
```

With x86:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 92

Mean (80%) duration: 100.563

Sample data [100]
........................
80 82 82 82 82 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84 84
........................
190 190 190 190 190 192 192 192 192 192 192 192 192 194 194 194 194 194 194 194 196 196 196 196 196 196 198 198 198 198 198 198 200 200 200 200 200 202 202 202 202 202 204 204 204 204 206 208 208 208 208 210 210 210 214 214 218 222 222 224 224 224 226 232 232 234 250 254 256 256 260 272 272 274 274 278 280 284 284 288 292 302 334 346 350 386 388 412 420 424 474 554 12816 14964 21222 22212 22468 23188 23950 26676
........................

x= 2.47525e+07 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learned that everything between the 

```c++
const auto startTime = c_ext_getCPUClock();
```

and 
```c++
const auto stopTime = c_ext_getCPUClock();
```

While an iteration loop can be used to track the timing.

*Did you make any mistakes?* 
No. The results were as expected with x64 being a bit faster than x86.

*In what way has your knowledge improved?*
I understand the core concept in how the code for testing the timing works.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q3. Conditionals

**Question:**

In the lectures you will have covered the if, switch and ?: conditional statements
Add each in turn to the payload to try and identify any performance differences.

Are the results as you expected?

Is there anything you need to write in your log book for future reference?

**Solution:**

- For an if statement (with out else):
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX += (j % 13) * 1.0001;

	if (j % 5) {
		dummyX *= 1.0001;
	}
}
// END payload
```

- For an if statement:
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX += (j % 13) * 1.0001;

	if (j % 5) {
		dummyX *= 1.0001;
	}else{
		dummyX *= 0.0;
	}
}
// END payload
```

- For a switch statement:
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX += (j % 13) * 1.0001;

	switch (j % 5) {
	case 0:
		dummyX *= 1.0001;
		break;
	case 1:
		dummyX /= 1.0001;
		break;
	default:
		dummyX += 1.0001;
		break;
	}
}
// END payload
```

- For an ? statement:
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX +=  (j % 13) * 1.0001;

	dummyX *= (j % 5) ? 1.0001 : 0.0;
}
// END payload
```

**Sample output:**

- For an if statement (with out else):
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 136

Mean (80%) duration: 136.798

Sample data [100]
........................
130 130 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132 132
........................
266 268 268 268 268 268 268 268 268 268 270 270 270 270 270 270 272 272 272 274 274 274 274 276 276 276 276 276 276 276 278 278 282 282 286 288 288 290 294 294 294 294 296 296 310 314 318 318 320 320 320 324 324 326 328 328 330 330 332 336 336 338 342 342 344 344 346 346 346 352 352 354 356 364 364 372 374 374 394 412 414 424 424 458 466 480 484 506 2148 11872 12072 12424 14182 16042 16528 19610 20928 23098 23836 28636
........................

x= 3.16741e+178 (dummy value - ignore)
```

- For an if statement (with else):
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 148

Mean (80%) duration: 147.413

Sample data [100]
........................
142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142 142
........................
278 278 278 278 278 278 280 280 280 280 282 282 282 284 284 284 284 286 286 286 288 288 288 290 290 290 290 290 292 294 294 294 294 296 296 296 296 298 298 302 304 304 306 306 308 316 318 320 322 322 324 324 326 328 330 332 334 334 336 336 336 340 344 346 350 350 354 354 358 358 358 360 360 362 364 366 374 404 406 410 416 428 432 482 494 496 578 616 778 11740 16222 16724 17532 17726 19740 19914 20296 25590 30792 158132
........................

x= 18.0058 (dummy value - ignore)
```

- For an switch statement:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 176

Mean (80%) duration: 178.447

Sample data [100]
........................
170 170 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172 172
........................
324 324 326 326 326 326 326 328 328 332 332 332 334 334 334 334 336 338 338 344 344 346 346 348 348 348 352 354 354 356 356 356 360 362 362 362 364 364 364 366 366 370 370 374 378 378 380 382 382 382 384 384 384 384 386 392 394 394 396 404 406 414 416 416 418 422 424 424 426 442 458 464 486 498 516 518 522 524 524 528 534 538 542 542 556 560 578 808 1466 10270 11878 15998 16460 17724 17752 18052 19308 20240 29588 34140
........................

x= 2.77523e+07 (dummy value - ignore)
```

- For an ? statement:
```
Number of iterations: 250000

Overhead duration: 78

Median duration: 146

Mean (80%) duration: 145.372

Sample data [100]
........................
140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140 140
........................
276 276 276 276 276 276 278 278 278 278 280 280 282 282 282 284 286 286 286 286 286 288 290 290 290 292 292 294 294 296 296 296 298 298 302 302 310 312 312 314 318 322 322 322 322 322 324 326 328 328 328 330 332 336 336 336 336 338 344 344 346 346 348 348 352 354 354 354 356 356 358 366 370 370 380 390 414 450 450 450 452 454 466 468 468 480 484 488 498 970 13274 14156 16198 17038 18014 18128 18332 21006 21900 27702
........................

x= 18.0058 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I noticed the difference in timing among the three tests. And the if statement is the fastest one, the branching is less expected But when I add an else the increase is quite significant.

*Did you make any mistakes?*
Yes, I had to run this test a lot and rewrite it becasuse I kept making the testing way to diferent, like comparing the statemtns with different operations.

*In what way has your knowledge improved?*
In testing about brnaching I noticed how  the more decision and paths the CPU has to make, the more slow it gets. Even if its just a small increase in the number of operations like an else.

**Questions:**

*Is there anything you would like to ask?*
No 

#### Q4. Branch prediction

**Question:**

Add a piece of code to the payload that demonstrates when branch prediction is working well and when branch prediction is failing.

Discuss the problems in creating this code with another student.

**Solution:**

- Branching prediction working well:
```c++
// BEGIN payload
for (auto j = 0; j < 40; j++) {
    if (j < 20) { // Always true for j = 0..19, always false for j = 20..39
        dummyX += 1.0;
    } else {
        dummyX -= 1.0;
    }
}
// END payload
```

- Branching prediction not working well:
```c++
#include <random>

<...>

// BEGIN payload
static std::mt19937 rng(42); // Fixed seed for repeatability
static std::uniform_int_distribution<int> dist(0, 1);
for (auto j = 0; j < 40; j++) {
    if (dist(rng)) { // Randomly true or false
        dummyX += 1.0;
    } else {
        dummyX -= 1.0;
    }
}
// END payload
```

**Sample output:**

- Branching prediction working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 128

Mean (80%) duration: 128.066

Sample data [100]
........................
122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124
........................
160 160 162 162 162 162 162 162 166 166 166 166 166 166 168 168 168 168 170 170 170 172 172 172 172 172 174 174 176 176 176 180 182 184 184 184 186 186 188 188 190 192 192 192 192 194 194 196 198 202 202 204 210 236 238 240 260 260 262 264 264 280 282 282 282 284 284 286 288 290 290 310 316 322 322 340 346 366 380 388 410 410 412 416 446 460 472 488 500 504 512 520 528 15816 17138 17302 21002 21160 28194 33554
........................

x= 1 (dummy value - ignore)
```

- Branching prediction not working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 1084

Mean (80%) duration: 1099.72

Sample data [100]
........................
660 664 670 674 682 684 684 692 692 694 694 696 700 702 702 702 704 704 706 708 710 710 712 712 712 714 716 718 718 718 718 718 720 720 722 722 722 724 724 724 726 726 726 726 728 728 728 730 730 732 732 732 732 734 734 734 734 734 734 736 736 736 736 736 738 738 738 740 740 740 740 740 740 740 740 740 740 740 740 740 742 742 742 742 742 742 742 742 742 742 742 744 744 744 744 746 746 746 748 748
........................
5552 5552 5554 5556 5558 5558 5558 5558 5560 5560 5560 5562 5562 5566 5570 5570 5570 5572 5578 5578 5578 5578 5580 5582 5584 5584 5596 5604 5604 5606 5608 5608 5614 5618 5620 5622 5666 5686 5692 5694 5722 7754 9052 9128 9236 9284 9310 10258 11898 12782 12910 13032 13732 14056 14294 14492 14834 15958 16328 16632 16738 16800 17198 17570 17764 18212 18218 18294 18350 18478 18690 19230 19310 19510 21076 21742 21954 22252 22524 23036 23214 23646 23850 24744 27096 27436 27560 27632 27632 27880 27942 28282 28562 29584 30552 30690 30740 34224 34508 36838
........................

x= -529 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Yeah, you can clearly se the change in the median and mean of both tests.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In that branch prediction and predictable pattersn help the CPU to be more efficient. As I discussed with a classmate, modern CPUs are designed to predict the path of execution to keep their pipelines full and minimize stalls. When the CPU can accurately predict the outcome of a branch (like an if-else statement), it can continue executing instructions without waiting for the branch to be resolved. This is known as branch prediction.
When the branching is predictable, the CPU can make accurate predictions about which path will be taken, allowing it to pre-load instructions and data for that path. This leads to fewer pipeline stalls and better overall performance.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q5. Exiting a nested loop

**Question:**

During the lectures we discussed a number of methods for exiting a nested for loop.

1. Two conditions in each conditional section of the loops.  One for the loop control and the other as the exit condition
2. An additional if statement immediately following the inner loop to catch and propagate a break statement
3. A goto statement in the inner loop
4. A lambda function

Add each option to the payload section of the timing code and determine if there is any performance differences between each approach.

You have 4 methods to solve a common problem.  Do you have a preference, now that you’ve tested them?
If so make a note in your log book as to the pros and cons of each.

**Solution:**

- Two conditions in each conditional section of the loops:
```c++
// BEGIN payload
for (auto j = 0; (j < 20) && (dummyX > 1.00005000100001); j++) {
	dummyX = dummyX * 1.00001;
}
// END payload
```

- Additional if statement immediately following the inner loop to catch and propagate a break statement:
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX = dummyX * 1.00001;

	if(dummyX > 1.00005000100001) {
		break;
	}
}
// END payload
```

- A goto statement in the inner loop:
```c++
// BEGIN payload
for (auto j = 0; j < 20; j++) {
	dummyX = dummyX * 1.00001;

	if(dummyX > 1.00005000100001) {
		goto EndOfOuterLoop;
	}
}
// END payload
EndOfOuterLoop:
```

- A lambda function:
```c++
const auto loopFunction = [](auto x) {
	// Dummy function to have a payload
	for (auto j = 0; j < 20; j++) {
		x = x * 1.00001;
		if (x > 1.00005000100001) {
			return x;
		}
	}
	return x;
};

<...>

dummyX = loopFunction(dummyX);
```

**Sample output:**

- Two conditions in each conditional section of the loops:
```
Number of iterations: 250000

Overhead duration: 78

Median duration: 1

Mean (80%) duration: 1.0069

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
30 30 32 32 32 32 32 32 32 32 32 32 32 34 34 34 34 36 38 38 38 38 38 40 40 42 42 44 44 44 44 46 48 48 48 48 50 50 52 52 54 54 56 56 58 58 60 60 60 60 60 60 60 60 60 62 64 64 64 64 64 66 68 68 68 74 74 76 78 84 86 94 106 106 108 118 128 130 144 152 154 156 156 158 162 164 164 168 234 238 276 288 290 294 414 8816 9450 19312 24500 31368
........................

x= 1 (dummy value - ignore)


C:\Users\949145\GitHub\lab-b-JoseSerranoHull\x64\Release\Timing.exe (process 12404) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .
```

- Additional if statement immediately following the inner loop to catch and propagate a break statement:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.92276

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 28 28 28 28 28 30 30 30 30 30 30 32 32 32 32 32 32 32 32 32 34 34 34 34 34 34 36 36 36 36 36 38 38 40 40 42 42 42 42 44 44 44 44 48 50 54 54 58 60 60 60 60 62 62 62 62 62 62 64 66 68 68 68 68 70 70 76 76 78 82 92 114 116 152 162 164 170 172 174 174 206 236 238 240 270 312 558 2834 3544 3552 6052 9130 14004 17286 17326 17462 29716
........................

x= 12.1828 (dummy value - ignore)
```

- A goto statement in the inner loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 2.24039

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
68 68 68 68 70 70 70 70 70 72 72 72 72 74 76 76 76 76 76 78 78 78 78 80 80 80 82 84 86 86 88 88 90 92 94 98 98 98 100 106 106 108 112 112 112 114 114 116 116 120 124 126 128 128 130 130 130 132 132 134 134 136 140 140 142 144 150 152 152 154 154 154 156 158 160 160 166 166 166 168 170 172 174 200 204 204 236 236 236 244 254 260 286 286 292 430 1188 17304 25040 31238
........................

x= 12.1828 (dummy value - ignore)
```

- A lambda function:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.93567

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 26 26 26 26 26 26 26 26 26 26 28 28 28 28 28 28 28 28 28 28 28 30 30 30 30 30 30 30 30 32 32 32 32 32 32 34 34 34 36 38 38 40 40 40 42 44 46 46 46 48 48 50 52 52 52 54 54 56 56 56 56 60 60 60 60 62 62 62 64 64 66 66 68 70 78 82 88 96 108 108 112 142 148 150 158 162 164 170 172 174 238 248 258 280 13730 16530 31216
........................

x= 12.1828 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
After reading all the results of these tests I came to the conclution that the dual conditional in the for loop is the fastest way to exit while the goto al thought simple to implement it has alot of caveats and it's the slowest.

*Did you make any mistakes?*
No as far as I know.

*In what way has your knowledge improved?*
As in understanding and practicing the  many techiques to exit a nested loop. And which ones are better in terms of performance.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q6. Range based loops

**Question:**

C++ 11 introduced range based loops.  How do these compare in performance to standard loops.  
You know the drill, by now.  Add both types of loop to the payload and determine what if any performance difference that you find.

**Solution:**
- C++11 range based loop:
```c++
#include <array>

<...>

// BEGIN payload
for (int& _ : std::array<int, 20>{}) {
	dummyX = dummyX * 1.00001;
}
// END payload
```

- C++14 range based loop:
```c++
#include <array>

<...>

// BEGIN payload
for (auto& _ : std::array<int, 20>{}) {
    dummyX = dummyX * 1.00001;
}
// END payload
```

**Sample output:**
- C++11 range based loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 60

Mean (80%) duration: 59.9242

Sample data [100]
........................
56 56 56 56 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
........................
88 88 88 88 88 88 88 88 90 90 90 90 90 90 92 94 94 94 96 96 96 96 96 96 98 98 100 100 100 100 102 102 102 104 106 106 106 106 108 108 110 110 114 114 114 114 116 116 118 118 120 120 120 122 122 122 122 124 124 126 126 128 128 128 128 136 154 170 172 182 190 196 198 202 202 206 208 212 212 214 218 226 240 240 242 292 328 352 352 354 354 358 358 360 370 478 16974 17474 17740 23210
........................

x= 5.18341e+21 (dummy value - ignore)
```

- C++14 range based loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 60

Mean (80%) duration: 60.1881

Sample data [100]
........................
56 56 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
........................
104 104 104 106 108 108 108 110 110 112 112 112 112 112 114 116 116 116 118 118 118 118 118 120 122 122 124 126 126 126 126 128 130 130 132 132 132 134 136 142 144 146 148 150 150 152 156 156 160 162 166 168 186 186 188 192 192 194 196 198 198 202 204 204 208 208 208 208 212 218 220 222 226 226 226 232 236 240 244 246 252 254 264 264 272 290 312 314 324 346 352 370 394 914 1518 12390 16712 17882 19280 87104
........................

x= 5.18341e+21 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Interesting results, I thought that the C++14 would be faster but it seems that both are almost identical in performance.

*Did you make any mistakes?*
Yes, I struggled a bit in how ti implement the range based loops.

*In what way has your knowledge improved?*
In that I practiced how to create ranged based loops and the difference between C++11 and C++14 and their timing.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q7. Architecture

**Question:**

Since you're programming on a 64 bit system, you have the option of producing either 32 bit (x86) or 64 bit (x64) code.
Rerun some of the previous experiments and compare between x86 and x64.  Are there any noticeable differences?

You're now probably reaching the limit of what you can do with the timing code.  The C++ compiler hides a lot from you.  The next stage is to start to look at the assembly language, generated by the compiler, to try and understand what is really happening.  We'll start this in the next lecture.

**Sample output:**

- Rerun Q4 with x86 and x64 architectures:
- - x86 architecture:

- - - Branching prediction working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 138

Mean (80%) duration: 137.343

Sample data [100]
........................
126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 126 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128 128
........................
184 184 184 184 184 186 186 186 186 186 186 186 186 186 186 186 186 186 186 186 188 188 188 188 190 190 190 190 190 190 190 190 192 192 192 192 194 194 196 196 198 198 198 198 198 198 198 200 204 204 204 206 206 206 210 212 218 222 224 224 230 232 234 246 252 256 256 258 276 278 286 292 296 298 308 316 340 386 394 400 442 454 460 524 558 606 708 836 1288 3340 18218 20948 21602 22588 22964 23344 24686 26936 30496 215194
........................

x= 1 (dummy value - ignore)
```

- - - Branching prediction not working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 1118

Mean (80%) duration: 1122.05

Sample data [100]
........................
708 712 714 718 732 736 738 740 744 746 748 748 750 750 756 758 760 760 762 764 764 766 768 768 768 770 770 770 770 772 772 772 772 774 774 774 776 776 776 776 776 778 778 778 778 778 778 778 778 780 780 780 780 782 782 782 782 782 782 784 784 784 784 784 784 786 786 788 788 788 790 790 790 792 792 792 792 792 792 794 794 794 794 794 794 796 796 796 796 796 796 796 796 796 796 796 796 796 796 798
........................
6008 6012 6012 6014 6026 6030 6030 6030 6038 6046 6048 6056 6062 6070 6086 6086 6090 6094 6096 6104 6108 6120 6122 6130 6140 6144 6148 6158 6180 6216 6222 6244 6322 6456 6532 6550 7510 7638 7672 8028 8256 8962 8998 10906 10970 11074 12136 12250 12510 14434 14484 14762 15048 15134 16316 16364 16980 17482 18102 18182 18304 18432 18546 18724 18864 18890 18986 19094 19142 19556 19670 19932 20014 20172 20240 20524 20850 21050 21234 21264 22306 22548 23072 23104 23370 23736 23844 24916 25356 25816 27804 27926 29394 29448 30016 30166 32204 32970 37414 83202
........................

x= -529 (dummy value - ignore)
```

- - x64 architecture:
- - - Branching prediction working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 128

Mean (80%) duration: 128.066

Sample data [100]
........................
122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 122 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124 124
........................
160 160 162 162 162 162 162 162 166 166 166 166 166 166 168 168 168 168 170 170 170 172 172 172 172 172 174 174 176 176 176 180 182 184 184 184 186 186 188 188 190 192 192 192 192 194 194 196 198 202 202 204 210 236 238 240 260 260 262 264 264 280 282 282 282 284 284 286 288 290 290 310 316 322 322 340 346 366 380 388 410 410 412 416 446 460 472 488 500 504 512 520 528 15816 17138 17302 21002 21160 28194 33554
........................

x= 1 (dummy value - ignore)
```

- - - Branching prediction not working well:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 1084

Mean (80%) duration: 1099.72

Sample data [100]
........................
660 664 670 674 682 684 684 692 692 694 694 696 700 702 702 702 704 704 706 708 710 710 712 712 712 714 716 718 718 718 718 718 720 720 722 722 722 724 724 724 726 726 726 726 728 728 728 730 730 732 732 732 732 734 734 734 734 734 734 736 736 736 736 736 738 738 738 740 740 740 740 740 740 740 740 740 740 740 740 740 742 742 742 742 742 742 742 742 742 742 742 744 744 744 744 746 746 746 748 748
........................
5552 5552 5554 5556 5558 5558 5558 5558 5560 5560 5560 5562 5562 5566 5570 5570 5570 5572 5578 5578 5578 5578 5580 5582 5584 5584 5596 5604 5604 5606 5608 5608 5614 5618 5620 5622 5666 5686 5692 5694 5722 7754 9052 9128 9236 9284 9310 10258 11898 12782 12910 13032 13732 14056 14294 14492 14834 15958 16328 16632 16738 16800 17198 17570 17764 18212 18218 18294 18350 18478 18690 19230 19310 19510 21076 21742 21954 22252 22524 23036 23214 23646 23850 24744 27096 27436 27560 27632 27632 27880 27942 28282 28562 29584 30552 30690 30740 34224 34508 36838
........................

x= -529 (dummy value - ignore)
```

- Rerun Q5 with x86 and x64 architectures:
- - x86 architecture:
- - - Two conditions in each conditional section of the loops:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 4

Mean (80%) duration: 3.63599

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 26 26 26 26 26 26 26 26 28 28 28 28 28 28 28 28 28 28 30 30 30 30 30 30 30 32 32 32 32 34 34 34 34 36 36 36 36 36 38 38 40 40 42 42 42 42 44 44 46 46 46 48 48 50 50 52 52 52 52 54 56 56 56 56 58 58 58 58 60 62 62 62 62 68 68 70 72 74 76 76 80 84 86 90 112 146 152 158 170 174 238 238 238 298 17286 18324 31166
........................

x= 1 (dummy value - ignore)
```

- - - Additional if statement immediately following the inner loop to catch and propagate a break statement:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 4

Mean (80%) duration: 4.91924

Sample data [100]
........................
2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
........................
36 36 36 38 38 38 38 38 38 40 40 40 40 40 40 40 40 40 42 42 42 42 42 42 42 44 44 44 44 46 46 46 46 46 46 48 48 50 50 50 50 52 52 54 54 54 56 58 58 58 58 58 58 60 60 62 62 62 62 62 64 64 64 64 64 66 66 66 66 66 66 68 68 68 70 74 76 78 78 80 86 86 96 96 96 100 102 130 132 138 156 164 204 302 316 316 4476 17078 21678 29144
........................

x= 12.1828 (dummy value - ignore)
```

- - - A goto statement in the inner loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 4

Mean (80%) duration: 5.02352

Sample data [100]
........................
1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
........................
42 42 44 46 46 46 48 48 48 48 48 48 48 48 50 50 50 52 54 54 56 56 56 58 60 60 60 62 62 62 62 62 62 62 62 64 64 64 64 64 66 66 66 68 68 70 70 70 72 72 72 74 76 76 78 78 80 82 82 82 86 110 114 132 132 134 134 136 136 142 152 154 158 192 202 222 246 262 292 294 296 300 314 2248 2266 2860 2870 3234 4538 4708 4730 4762 5060 5922 6764 9902 20614 23232 25774 28224
........................

x= 12.1828 (dummy value - ignore)
```

- - - A lambda function:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 4

Mean (80%) duration: 4.95195

Sample data [100]
........................
1 1 1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
........................
44 46 46 46 46 48 48 48 48 48 52 52 52 52 54 54 54 54 56 56 56 56 56 56 58 58 58 58 58 58 60 60 60 62 62 62 62 62 62 62 64 64 66 66 66 66 66 68 68 68 68 70 70 70 72 72 72 74 76 76 76 82 82 82 84 86 88 90 92 116 118 122 130 132 134 140 144 146 152 152 156 156 160 162 164 170 170 172 178 242 284 288 292 296 302 304 380 608 17482 30128
........................

x= 12.1828 (dummy value - ignore)
```

- - x64 architecture:
- - - Two conditions in each conditional section of the loops:
```
Number of iterations: 250000

Overhead duration: 78

Median duration: 1

Mean (80%) duration: 1.0069

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
30 30 32 32 32 32 32 32 32 32 32 32 32 34 34 34 34 36 38 38 38 38 38 40 40 42 42 44 44 44 44 46 48 48 48 48 50 50 52 52 54 54 56 56 58 58 60 60 60 60 60 60 60 60 60 62 64 64 64 64 64 66 68 68 68 74 74 76 78 84 86 94 106 106 108 118 128 130 144 152 154 156 156 158 162 164 164 168 234 238 276 288 290 294 414 8816 9450 19312 24500 31368
........................

x= 1 (dummy value - ignore)


C:\Users\949145\GitHub\lab-b-JoseSerranoHull\x64\Release\Timing.exe (process 12404) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .
```

- - - Additional if statement immediately following the inner loop to catch and propagate a break statement:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.92276

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 28 28 28 28 28 30 30 30 30 30 30 32 32 32 32 32 32 32 32 32 34 34 34 34 34 34 36 36 36 36 36 38 38 40 40 42 42 42 42 44 44 44 44 48 50 54 54 58 60 60 60 60 62 62 62 62 62 62 64 66 68 68 68 68 70 70 76 76 78 82 92 114 116 152 162 164 170 172 174 174 206 236 238 240 270 312 558 2834 3544 3552 6052 9130 14004 17286 17326 17462 29716
........................

x= 12.1828 (dummy value - ignore)
```

- - - A goto statement in the inner loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 2.24039

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
68 68 68 68 70 70 70 70 70 72 72 72 72 74 76 76 76 76 76 78 78 78 78 80 80 80 82 84 86 86 88 88 90 92 94 98 98 98 100 106 106 108 112 112 112 114 114 116 116 120 124 126 128 128 130 130 130 132 132 134 134 136 140 140 142 144 150 152 152 154 154 154 156 158 160 160 166 166 166 168 170 172 174 200 204 204 236 236 236 244 254 260 286 286 292 430 1188 17304 25040 31238
........................

x= 12.1828 (dummy value - ignore)
```

- - - A lambda function:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.93567

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 26 26 26 26 26 26 26 26 26 26 28 28 28 28 28 28 28 28 28 28 28 30 30 30 30 30 30 30 30 32 32 32 32 32 32 34 34 34 36 38 38 40 40 40 42 44 46 46 46 48 48 50 52 52 52 54 54 56 56 56 56 60 60 60 60 62 62 62 64 64 66 66 68 70 78 82 88 96 108 108 112 142 148 150 158 162 164 170 172 174 238 248 258 280 13730 16530 31216
........................

x= 12.1828 (dummy value - ignore)
```

- Rerun Q6 with x86 and x64 architectures:
- - x86 architecture:
- - - C++11 range based loop:
```
Number of iterations: 250000

Overhead duration: 78

Median duration: 64

Mean (80%) duration: 64.3612

Sample data [100]
........................
60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 60 62 62 62 62 62 62 62 62 62 62 62 62
........................
86 86 86 86 86 86 86 86 86 86 86 86 86 86 86 86 88 88 88 88 88 88 88 88 88 88 88 90 90 90 90 90 90 92 92 92 92 92 94 94 96 96 96 98 98 98 98 100 100 100 100 102 102 102 102 102 104 104 104 104 104 104 108 108 108 108 110 110 112 114 116 120 122 124 126 130 132 132 132 136 138 140 150 190 198 208 214 216 218 218 224 282 356 358 368 370 15492 17874 30734 45114
........................

x= 5.18341e+21 (dummy value - ignore)
```

- - - C++14 range based loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 66

Mean (80%) duration: 66.4058

Sample data [100]
........................
62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62
........................
90 90 92 92 92 92 92 92 94 94 94 94 94 94 94 94 94 96 96 96 96 98 98 98 100 100 100 100 102 102 102 102 102 102 102 102 104 106 106 106 108 108 108 110 112 112 114 114 116 118 118 118 120 120 120 122 124 124 126 126 128 128 128 128 130 134 134 136 138 140 140 140 140 148 174 174 190 192 192 194 196 208 208 218 218 222 226 228 234 252 256 308 352 352 352 366 12966 20122 23954 33180
........................

x= 5.18341e+21 (dummy value - ignore)
```

- - x64 architecture:
- - - C++11 range based loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 60

Mean (80%) duration: 59.9242

Sample data [100]
........................
56 56 56 56 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
........................
88 88 88 88 88 88 88 88 90 90 90 90 90 90 92 94 94 94 96 96 96 96 96 96 98 98 100 100 100 100 102 102 102 104 106 106 106 106 108 108 110 110 114 114 114 114 116 116 118 118 120 120 120 122 122 122 122 124 124 126 126 128 128 128 128 136 154 170 172 182 190 196 198 202 202 206 208 212 212 214 218 226 240 240 242 292 328 352 352 354 354 358 358 360 370 478 16974 17474 17740 23210
........................

x= 5.18341e+21 (dummy value - ignore)
```

- - - C++14 range based loop:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 60

Mean (80%) duration: 60.1881

Sample data [100]
........................
56 56 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58 58
........................
104 104 104 106 108 108 108 110 110 112 112 112 112 112 114 116 116 116 118 118 118 118 118 120 122 122 124 126 126 126 126 128 130 130 132 132 132 134 136 142 144 146 148 150 150 152 156 156 160 162 166 168 186 186 188 192 192 194 196 198 198 202 204 204 208 208 208 208 212 218 220 222 226 226 226 232 236 240 244 246 252 254 264 264 272 290 312 314 324 346 352 370 394 914 1518 12390 16712 17882 19280 87104
........................

x= 5.18341e+21 (dummy value - ignore)
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I reviewed most of the exercises and as expected  x64 is faster than x86. The differences in time are not that big but they are there. I also learned that branching prediction is a thing and it can affect performance quite a bit.

*Did you make any mistakes?*
Yes, I had to rerun almost everything because I forgot to change the architecture in the properties of the project.

*In what way has your knowledge improved?*
That it is important to verfy the architecture when compiling and that branching prediction, good practices in optimization are things that can affect performance.

**Questions:**

*Is there anything you would like to ask?*
No.

### Final Reflection
After I finished I reviewed most of what I did on the lab today, and I believe that for the moment I understand the process of the tests on these benchmarks on timing.

And of course, I learned on a real, proof tested way that x64 is faster than x86, and that branching prediction is a thing that can affect performance quite a bit. That even the differences in selecting how to apporach a loop nd exiting one can affect performance.

---

## Lab C
### Week 3 - Lab C

17 Oct 2025

#### Introduction
This lab introduces you to debugging, as well as exploring some of the language constructs you have covered in recent lectures e.g. I/O, bitwise operators and assembly.

#### Q1. Debugging

**Question:**

Locate the Solution Explorer within Visual Studio and select the Debugging project. Right-click on this project and select Build. This should compile and link the project. Now run the Debugging program. The program does not give the correct output. By clicking in the left-hand margin of the cpp file, set a breakpoint online.

```c++
auto equals1 = 0;
```

A red circle represents a breakpoint. Now execute the program within the debugger by pressing F5. Notice that the program stops on the breakpoint and is represented by a yellow arrow. By pressing F10 you can single-step through your code.

From the menu select debug, then windows, then auto, local and this. These 3 new windows show you the state of the variables within your program.

Continue stepping through your code and determine what the problem is with the program.

Suggest a solution to make the program execute correctly.

**Solution:**

```c++
#include <iostream>

int main(int argc, char** argv) {
	
	std::cout << "Enter a list of integers, and terminating with a letter" << std::endl;

	auto equals1 = 0u;
	auto equals2 = 0u;
	auto equals3 = 0u;

	std::string input;
	std::cin >> input;

	for (char ch : input) {
		if (ch == '1') equals1++;
		else if (ch == '2') equals2++;
		else if (ch == '3') equals3++;
	}

	std::cout << equals1 << " inputs equals 1" << std::endl;
	std::cout << equals2 << " inputs equals 2" << std::endl;
	std::cout << equals3 << " inputs equals 3" << std::endl;

	return 0;
}
```

**Sample input:**

```
12343a
```

**Sample output:**

```
1 inputs equals 1
1 inputs equals 2
2 inputs equals 3
```

**Reflection:**
*Reflect on what you have learnt from this exercise.*
That debugging is a very useful tool to find mistakes in code, and that stepping through the code while checking the values of variables can help you understand where the logic is failing.

*Did you make any mistakes?* 
Not really, I just had to understand how to use the debugger properly and think of a solution.

*In what way has your knowledge improved?*
In that std::cin >> value reads input until a whitespace is found, but if you put together values like "12343a" it will read int as 12343 and not separatly. But as a string you can separate it in char.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q2. Bitwise

**Question:**

Create a new empty c++ project in the current Visual Studio solution called Bitwise.

Write a program to read four separate 32-bit integers (red, green, blue, and alpha) and encode them into a single 32-bit value. Output this 32-bit value. Verify that the results are correct by taking the 32-bit value and extracting and outputting the separate integers.

> Hint: You'll need to use the bitwise operators.  The debugger may help you understand the data format.
You can also stream hex and binary using `std::hex` and `std::bitset` respectively

```c++
const auto x = 1234u;
std::cout << std::hex << x << " " << std::bitset<32>(x) << std::endl;
```

**Solution:**

```c++
#include <iostream>
#include <bitset>

int main()
{
    unsigned int red = 0, green = 0, blue = 0, alpha = 0;

    std::cout << "Enter red (0-255): ";
    std::cin >> red;
    std::cout << "Enter green (0-255): ";
    std::cin >> green;
    std::cout << "Enter blue (0-255): ";
    std::cin >> blue;
    std::cout << "Enter alpha (0-255): ";
    std::cin >> alpha;

    // Pack into a single 32-bit value: ARGB

    /*
        Alpha: bits 24–31
        Red: bits 16–23
        Green: bits 8–15
        Blue: bits 0–7
     */

    unsigned int packed = (red << 24) | (green << 16) | (blue << 8) | alpha;

    std::cout << "Packed value (hex): 0x" << std::hex << packed << std::endl;
    std::cout << "Packed value (bits): " << std::bitset<32>(packed) << std::endl;

    return 0;
}
```

**Sample Input:**
```
Enter red (0-255): 34
Enter green (0-255): 6
Enter blue (0-255): 87
Enter alpha (0-255): 1
```

**Sample output:**

```
Packed value (hex): 0x22065701
Packed value (bits): 00100010000001100101011100000001
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt about bitwise instructions and techniques that are required in order to for example grab a couple of bits from a number, or to pack several numbers into one. In this case a color value made of four different values.

*Did you make any mistakes?* 
Not really, I just had to think a bit about how to shift the bits and combine them properly.

*In what way has your knowledge improved?*
That these bitwise operations are very useful when dealing with low level data manipulation, and that understanding how to use them can help in various programming tasks. But more importantly it was an example of techniques that are useful in for example game development and computer graphics, where performance and memory usage are critical.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q3. Parsing

**Question:**

Select and build the Parser project. This application scans through a C++ file looking for a particular variable. When and if it finds the variable it returns the word and line position of that variable. Comments are ignored. Familiarize yourself with this program and then modify the program to make the loop structures more efficient and easier to maintain.

Are you sure that the changes you have made are more efficient?  Use the timer code from the previous lab to test the code.

Be very careful in adding too large a payload to the timer code, as it suspends virtually all OS functionality when running the test.

> Hint: The break and continue statements can reduce the number of tests in a loop conditional.

**Solution:**

```c++
#include <iostream>
#include <fstream>
#include <strstream>
#include <string>

/*
* A program that scans through a C++ file looking for a particular variable
* When the variable is found the line number and position within the line are returned
* Comments are ignored
*/

int main(int argc, char** argv) {
	std::string variable;
	std::cout << "Enter a search variable" << std::endl;
	std::cin >> variable;

	std::ifstream fin("sample.txt");
	auto found = false;
	auto position = -1;
	int line;

	for (line = 1; !fin.eof() && !found; line++) {

		char lineBuffer[100];
		fin.getline(lineBuffer, sizeof(lineBuffer));
		const auto lengthOfLine = static_cast<int>(fin.gcount());

		std::istrstream sin(lineBuffer, lengthOfLine - 1);  // Removes end of line character

		std::string word;
		for (position = 1; (sin >> word) && !found && !((word[0] == '/') && (word[1] == '/')); position++) {
			if (word == variable) {
				found = true;
				break; // Exit the inner loop when found
			}
		}

		if (found) {
			break; // Exit the outer loop as well
		}
	}

	if (found)
		std::cout << variable << " appears as the " << position - 1 << " word on line number " << line - 1 << std::endl;
	else
		std::cout << variable << " does not appear in the file" << std::endl;

	return 0;
}
```

**Sample input:**
```
main
```

**Sample output:**

Normal timming

```
Enter a search variable
main
Number of iterations: 250000

Overhead duration: 78

Median duration: 8

Mean (80%) duration: 7.90355

Sample data [100]
........................
4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4
........................
38 38 38 38 38 38 38 38 38 38 40 40 40 42 42 42 42 44 44 44 44 44 44 44 46 46 46 46 46 48 48 48 50 50 52 52 52 54 54 56 56 56 58 60 60 62 62 62 64 64 64 66 68 68 68 68 68 70 72 72 76 78 78 80 80 80 80 84 86 110 116 122 138 142 152 156 156 158 160 164 164 164 192 192 194 210 236 238 242 248 276 314 428 536 564 834 13404 17570 22650 293000
........................

x= 1 (dummy value - ignore)

main appears as the 2 word on line number 0

C:\Users\949145\GitHub\lab-b-JoseSerranoHull\x64\Release\Timing.exe (process 26348) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .
```

With changes
```
Enter a search variable
main
Number of iterations: 250000

Overhead duration: 80

Median duration: 4

Mean (80%) duration: 4.76957

Sample data [100]
........................
1 1 1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
........................
32 34 34 34 34 34 34 34 34 34 34 34 34 36 36 38 38 38 38 38 40 42 44 46 46 48 48 50 50 50 52 52 54 54 54 56 56 58 58 60 62 62 62 62 62 62 64 64 64 66 68 68 70 70 70 72 72 72 74 74 74 74 78 82 84 86 90 96 100 104 104 106 118 118 128 138 140 150 158 158 160 182 186 194 198 198 200 206 246 256 290 292 300 306 382 476 15932 16318 19368 240816
........................

x= 1 (dummy value - ignore)

main appears as the 1 word on line number 0

C:\Users\949145\GitHub\lab-b-JoseSerranoHull\x64\Release\Timing.exe (process 29124) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt that breaking and continuing loops can help improve performance by reducing the number of checks that need to be made in each iteration. By exiting loops early when a condition is met, we can avoid unnecessary iterations and thus make the code more efficient.

*Did you make any mistakes?*
I want to believe that I did well the expermients on timing.

*In what way has your knowledge improved?*
That using techniques for optimization and experimenting on timing can help understand the performance implications of different coding styles and structures.

**Questions:**

*Is there anything you would like to ask?*
Are my experiments well put together?

```c++
#include <iostream>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <fstream>
#include <strstream>
#include <string>

#include <array>

// Useful link https://www.strchr.com/performance_measurements_with_rdtsc

extern "C"
{
#ifdef _WIN64
	INT64 c_ext_getCPUClock();
#else
	INT32 c_ext_getCPUClock();
#endif
}

int main(int, char**) {

	std::string variable;
	std::cout << "Enter a search variable" << std::endl;
	std::cin >> variable;

	std::ifstream fin("sample.txt");
	auto found = false;
	auto position = -1;
	int line;

	// Setup experiment
	const auto numOfIterations = 250000;
	std::cout << "Number of iterations: " << numOfIterations << std::endl;

	const auto processorInfinity = 2;
	auto* const thread = GetCurrentThread();
	auto* const process = GetCurrentProcess();

	// Set thread and process priorities to very high
	if (!SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL)) {
		SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
	}
	if (!SetPriorityClass(process, REALTIME_PRIORITY_CLASS)) {
		SetPriorityClass(process, HIGH_PRIORITY_CLASS);
	}
	SetProcessAffinityMask(process, processorInfinity);
	SetThreadAffinityMask(thread, processorInfinity);

	// Measure the overhead in calculating the time
	std::vector<DWORD> overheadTimes;
	for (auto i = 0; i < numOfIterations; i++) {
		const auto startTime = c_ext_getCPUClock();
		
		// Deliberately no payload here
		
		const auto stopTime = c_ext_getCPUClock();
		overheadTimes.push_back(static_cast<int>(stopTime - startTime));
	}

	// Check for over optimization
	if (numOfIterations != overheadTimes.size()) {
		std::cout << "\nERROR: Optimizer removed the code" << std::endl;
		return -1;
	}

	// Calculate the overhead
	sort(overheadTimes.begin(), overheadTimes.end());
	const auto overhead = overheadTimes[overheadTimes.size() / 2];
	std::cout << "\nOverhead duration: " << overhead << std::endl;

	// Dummy values to avoid the compiler optimizing out the test code
	auto dummyX = 1.0;

	// Run the actual experiment
	std::vector<DWORD> experimentTimes;
	for (auto i = 0; i < numOfIterations; i++) {
		const auto startTime = c_ext_getCPUClock();

		// BEGIN payload
		for (line = 1; !fin.eof() && !found; line++) {

			char lineBuffer[100];
			fin.getline(lineBuffer, sizeof(lineBuffer));
			const auto lengthOfLine = static_cast<int>(fin.gcount());

			std::istrstream sin(lineBuffer, lengthOfLine - 1);  // Removes end of line character

			std::string word;
			for (position = 1; (sin >> word) && !found && !((word[0] == '/') && (word[1] == '/')); position++) {
				if (word == variable) {
					found = true;
					break; // Exit the outer loop as well
				}
			}

			if (found) {
				break; // Exit the outer loop as well
			}
		}
		// END payload

		const auto stopTime = c_ext_getCPUClock();
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
		experimentTimes.push_back(duration > 0 ? duration : 1);
	}

	// Reset the thread priorities
	SetThreadPriority(thread, THREAD_PRIORITY_IDLE);
	SetPriorityClass(process, IDLE_PRIORITY_CLASS);

	// Check for over optimization
	if (numOfIterations != experimentTimes.size()) {
		std::cout << "\nERROR: Optimizer removed the code" << std::endl;
		return -1;
	}

	// Calculate the median
	sort(experimentTimes.begin(), experimentTimes.end());
	std::cout << "\nMedian duration: " << experimentTimes[experimentTimes.size() / 2] << std::endl;

	// Calculate the mean
	auto sum = 0.0;
	const auto lo = static_cast<unsigned int>(experimentTimes.size() * 0.1f);
	const auto hi = static_cast<unsigned int>(experimentTimes.size() * 0.9f); 
	for (auto i = lo; i < hi; i++)
		sum += experimentTimes[i];
	std::cout << "\nMean (80%) duration: " << (sum / (hi - lo)) << std::endl;

	// Output low and high samples
	const auto sampleSize = 100u;
	std::cout << "\nSample data [" << sampleSize << "]" << std::endl;
	std::cout << "........................" << std::endl;
	for (auto i = 0u; (i < sampleSize) && (i < experimentTimes.size()); i++)
		std::cout << experimentTimes[i] << " ";
	std::cout << "\n........................" << std::endl;
	for (auto i = experimentTimes.size() - sampleSize; i < experimentTimes.size(); i++)
		std::cout << experimentTimes[i] << " ";
	std::cout << "\n........................" << std::endl;

	// Print dummy x to avoid optimizer removing the loop
	std::cout << "\nx= " << dummyX << " (dummy value - ignore) \n" << std::endl;

	if (found)
		std::cout << variable << " appears as the " << position - 1 << " word on line number " << line - 1 << std::endl;
	else
		std::cout << variable << " does not appear in the file" << std::endl;

	return 0;
}
```

#### Q4. Quadratic

**Question:**

Select and build the Quadratic project.

Modify the quadratic program to:

1. Handle equal or imaginary roots
2. Output the roots to only 3 decimal places
3. Add an enum to store whether there is one root, two roots or imaginary roots.

Add the new code to the timing project (not the input or output code), and try to improve the program's efficiency.

Your log book should record anything you learn about the efficiency gains.

**Solution:**

```c++
#include <iostream>
#include <iomanip>
#include <cmath>
#include <complex>

int main(int argc, char** argv) {
    std::cout << "Enter the coefficients for a quadratic equation" << std::endl;
    double a, b, c;
    std::cin >> a >> b >> c;

    const double operand = b * b - 4.0 * a * c; // Check discriminant first
    const double two_a = 2.0 * a; // Precompute 2a

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "The roots of the equation "
        << a << "x^2 + " << b << "x + " << c << " are:\n";

    if (operand > 0) {
        // Two distinct real roots
        double root1 = (-b + std::sqrt(operand)) / two_a;
        double root2 = (-b - std::sqrt(operand)) / two_a;
        std::cout << root1 << " and " << root2 << std::endl;
    } else if (operand == 0) {
        // One repeated real root
        double root = -b / two_a;
        std::cout << root << " (repeated root)" << std::endl;
    } else {
        // Two complex roots
        std::complex<double> sqrt_disc(0, std::sqrt(-operand));
        std::complex<double> root1 = (-b + sqrt_disc) / two_a;
        std::complex<double> root2 = (-b - sqrt_disc) / two_a;
        std::cout << root1 << " and " << root2 << std::endl;
    }

    return 0;
}
```

But this would be before the optimization and just enableing it to handle equal and imaginary roots.

```c++
#include <iostream>
#include <cmath>
#include <complex>

/*
* Calculates the roots for a quadratic equation
* No account is taken of an imaginary root or floating point overflows
*/

// Now handles equal and imaginary roots

int main(int argc, char** argv) {
    std::cout << "Enter the coefficients for a quadratic equation" << std::endl;
    double a, b, c;
    std::cin >> a >> b >> c;

    double operand = b * b - 4.0 * a * c;

    if (operand > 0) {
        // Two distinct real roots
        double root1 = (-b + sqrt(operand)) / (2.0 * a);
        double root2 = (-b - sqrt(operand)) / (2.0 * a);
        std::cout << "The roots of the equation "
            << a << "x^2 + " << b << "x + " << c << "\n"
            << "are " << root1 << " and " << root2 << std::endl;
    } else if (operand == 0) {
        // One repeated real root
        double root = -b / (2.0 * a);
        std::cout << "The root of the equation "
            << a << "x^2 + " << b << "x + " << c << "\n"
            << "is " << root << " (repeated root)" << std::endl;
    } else {
        // Two complex (imaginary) roots
        std::complex<double> root1 = std::complex<double>(-b, sqrt(-operand)) / (2.0 * a);
        std::complex<double> root2 = std::complex<double>(-b, -sqrt(-operand)) / (2.0 * a);
        std::cout << "The roots of the equation "
            << a << "x^2 + " << b << "x + " << c << "\n"
            << "are " << root1 << " and " << root2 << std::endl;
    }

    return 0;
}
```

**Sample input:**
Without changes:
```c++
	double a = 2, b = 4, c = 6;
```

**Sample output:**
Without changes:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 1

Mean (80%) duration: 1.19087

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
34 34 36 36 36 36 36 36 36 36 36 36 38 38 38 40 40 40 40 42 42 42 42 44 44 44 44 44 46 46 46 48 48 48 48 48 50 50 50 50 52 52 52 52 54 54 54 54 54 56 56 56 56 56 58 58 60 60 60 60 60 62 62 62 62 64 64 66 68 68 74 76 76 78 80 86 90 92 110 112 120 126 126 126 126 136 136 138 148 150 158 164 164 174 242 290 296 324 19144 22880
........................

x= 1 (dummy value - ignore)
```

With changes:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 1

Mean (80%) duration: 1.15743

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
24 24 24 24 24 24 26 26 26 26 26 26 26 26 26 26 26 26 28 28 28 28 28 28 28 30 30 30 30 30 30 30 32 32 32 34 34 34 36 36 36 36 38 40 40 40 42 42 42 44 44 46 46 48 50 52 52 54 58 58 58 58 60 60 60 60 60 62 62 76 78 86 88 90 110 110 130 130 134 136 138 152 152 154 158 172 176 180 184 236 240 240 248 294 13226 15422 19200 23636 27750 34816
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt the importance of making calculations before and only once, to avoid redundant calculations that can slow down performance. For example, calculating the discriminant and 2a only once before the conditional checks.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
That optimizing code by reducing redundant calculations and improving logic flow can lead to better performance, especially in scenarios where the code is executed multiple times, such as in timing experiments.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q5. Assembly

**Question:**

Open your solution to the Parsing exercise in Visual Studio.  Set a break point in the code and execute the program, so that it halts on the break point.

Open up the Disassembly View: `Debug->Windows->Disassembly`

Open up the Register View: `Debug->Windows->Registers`

You should now see lines of C++ code below which are the associated lines of assembly.  Also in the bottom of the screen is a window containing all of the registers.

Whilst on the Disassembly View, single step through your code (using F10).  You will notice that you are not stepping through the C++, but are instead stepping through each line of assembly language.  If the execution of the assembly language results in a change of value in a register, then that register is highlighted red.

Familiarise yourself with other sections of your disassembled code.

>Hint: Use debug mode when first looking at assembly code, as it's considerably easier to relate lines of assembly code with a line of C++.  In Release mode, the optimizer produces efficient code, which inevitably moves around the assembly instructions so that the disassembler is unable to associate each instruction to a line of C++

**Sample output:**

Just as an example of the break point I put on line 31:
Release:
```
for (position = 1; (sin >> word) && !found && !((word[0] == '/') && (word[1] == '/')); position++) {
00007FF6ACE01B45  mov         esi,1  
00007FF6ACE01B4A  lea         rdx,[rbp+70h]  
00007FF6ACE01B4E  lea         rcx,[rbp+90h]  
00007FF6ACE01B55  call        std::operator>><char,std::char_traits<char>,std::allocator<char> > (07FF6ACE02330h)  
00007FF6ACE01B5A  mov         rcx,qword ptr [rax]  
00007FF6ACE01B5D  movsxd      rcx,dword ptr [rcx+4]  
00007FF6ACE01B61  add         rcx,rax  
00007FF6ACE01B64  call        qword ptr [__imp_std::ios_base::operator bool (07FF6ACE05198h)]  
00007FF6ACE01B6A  test        al,al  
00007FF6ACE01B6C  je          main+315h (07FF6ACE01C15h)  
00007FF6ACE01B72  nop         dword ptr [rax]  
00007FF6ACE01B76  nop         word ptr [rax+rax]  
00007FF6ACE01B80  lea         rax,[rbp+70h]  
00007FF6ACE01B84  mov         rbx,qword ptr [rbp+70h]  
00007FF6ACE01B88  mov         rdi,qword ptr [rbp+88h]  
00007FF6ACE01B8F  cmp         rdi,0Fh  
00007FF6ACE01B93  cmova       rax,rbx  
00007FF6ACE01B97  cmp         byte ptr [rax],2Fh  
00007FF6ACE01B9A  jne         main+2AEh (07FF6ACE01BAEh)  
00007FF6ACE01B9C  lea         rax,[rbp+70h]  
00007FF6ACE01BA0  cmp         rdi,0Fh  
00007FF6ACE01BA4  cmova       rax,rbx  
00007FF6ACE01BA8  cmp         byte ptr [rax+1],2Fh  
00007FF6ACE01BAC  je          main+320h (07FF6ACE01C20h)
```

on Debug:
```
for (position = 1; (sin >> word) && !found && !((word[0] == '/') && (word[1] == '/')); position++) {
00007FF7B037D195  mov         dword ptr [position],1  
00007FF7B037D19F  jmp         __$EncStackInitStart+188h (07FF7B037D1AFh)  
00007FF7B037D1A1  mov         eax,dword ptr [position]  
00007FF7B037D1A7  inc         eax  
00007FF7B037D1A9  mov         dword ptr [position],eax  
00007FF7B037D1AF  lea         rdx,[rbp+3B8h]  
00007FF7B037D1B6  lea         rcx,[rbp+290h]  
00007FF7B037D1BD  call        std::operator>><char,std::char_traits<char>,std::allocator<char> > (07FF7B0371037h)  
00007FF7B037D1C2  mov         qword ptr [rbp+638h],rax  
00007FF7B037D1C9  mov         rax,qword ptr [rbp+638h]  
00007FF7B037D1D0  mov         rax,qword ptr [rax]  
00007FF7B037D1D3  movsxd      rax,dword ptr [rax+4]  
00007FF7B037D1D7  mov         rcx,qword ptr [rbp+638h]  
00007FF7B037D1DE  add         rcx,rax  
00007FF7B037D1E1  mov         rax,rcx  
00007FF7B037D1E4  mov         rcx,rax  
00007FF7B037D1E7  call        qword ptr [__imp_std::ios_base::operator bool (07FF7B0392350h)]  
00007FF7B037D1ED  movzx       eax,al  
00007FF7B037D1F0  test        eax,eax  
00007FF7B037D1F2  je          __$EncStackInitStart+22Ch (07FF7B037D253h)  
00007FF7B037D1F4  movzx       eax,byte ptr [found]  
00007FF7B037D1FB  test        eax,eax  
00007FF7B037D1FD  jne         __$EncStackInitStart+22Ch (07FF7B037D253h)  
00007FF7B037D1FF  xor         edx,edx  
00007FF7B037D201  lea         rcx,[rbp+3B8h]  
00007FF7B037D208  call        std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator[] (07FF7B037112Ch)  
00007FF7B037D20D  movsx       eax,byte ptr [rax]  
00007FF7B037D210  cmp         eax,2Fh  
00007FF7B037D213  jne         __$EncStackInitStart+207h (07FF7B037D22Eh)  
00007FF7B037D215  mov         edx,1  
00007FF7B037D21A  lea         rcx,[rbp+3B8h]  
00007FF7B037D221  call        std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator[] (07FF7B037112Ch)  
00007FF7B037D226  movsx       eax,byte ptr [rax]  
00007FF7B037D229  cmp         eax,2Fh  
00007FF7B037D22C  je          __$EncStackInitStart+22Ch (07FF7B037D253h) 
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Relase mode creates code more efficient but it is harder to read and understand, as the optimizer rearranges instructions and removes some of the structure that is present in debug mode.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In how to use the disassembly view and how to relate assembly instructions to C++ code.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q6. Optimizer

**Question:**

Now that you’re more familiar with assembly code, jump back to your optimized Quadratic code in the Timer project.

Disassembly your solution, this time in Release mode.

Are you able to get a rough understanding of how the optimizer has compiled your C++?

You may find it easier to look at x86 code rather than x64.

Jot down in your log book anything significant that you discover

**Solution:**

Bit of code with x64
```
const auto stopTime = c_ext_getCPUClock();
00007FF60AE71358  call        c_ext_getCPUClock (07FF60AE722A0h)  
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
00007FF60AE7135D  sub         eax,ebx  
00007FF60AE7135F  sub         eax,esi  
		experimentTimes.push_back(duration > 0 ? duration : 1);
00007FF60AE71361  mov         ecx,1  
00007FF60AE71366  test        eax,eax  
00007FF60AE71368  cmovg       ecx,eax  
00007FF60AE7136B  mov         dword ptr [rbp+40h],ecx  
00007FF60AE7136E  mov         rdx,qword ptr [rbp-38h]  
00007FF60AE71372  cmp         rdx,qword ptr [rbp-30h]  
00007FF60AE71376  je          main+1E1h (07FF60AE71381h)  
00007FF60AE71378  mov         dword ptr [rdx],ecx  
00007FF60AE7137A  add         qword ptr [rbp-38h],4  
00007FF60AE7137F  jmp         main+1EEh (07FF60AE7138Eh)  
00007FF60AE71381  lea         r8,[rbp+40h]  
00007FF60AE71385  lea         rcx,[rbp-40h]  
00007FF60AE71389  call        std::vector<unsigned long,std::allocator<unsigned long> >::_Emplace_reallocate<unsigned long> (07FF60AE71DE0h)  
```

Bit of code with x86
```
const auto stopTime = c_ext_getCPUClock();
006C132E  call        c_ext_getCPUClock (06C21C0h)  
		const auto duration = static_cast<int>(stopTime - startTime - overhead);
006C1333  sub         eax,esi  
		experimentTimes.push_back(duration > 0 ? duration : 1);
006C1335  mov         ecx,1  
006C133A  sub         eax,ebx  
006C133C  test        eax,eax  
006C133E  cmovg       ecx,eax  
006C1341  mov         eax,dword ptr [ebp-18h]  
006C1344  mov         dword ptr [overhead],ecx  
006C1347  cmp         eax,dword ptr [ebp-14h]  
006C134A  je          main+1F4h (06C1354h)  
006C134C  mov         dword ptr [eax],ecx  
006C134E  add         dword ptr [ebp-18h],4  
006C1352  jmp         main+201h (06C1361h)  
006C1354  lea         ecx,[ebp-2Ch]  
006C1357  push        ecx  
006C1358  push        eax  
006C1359  lea         ecx,[experimentTimes]  
006C135C  call        std::vector<unsigned long,std::allocator<unsigned long> >::_Emplace_reallocate<unsigned long> (06C1CB0h)  
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt about the differences in assembly code generated for x86 and x64 architectures. As well as reading the bits of assembly code generated for the same C++ code on both architectures, I noticed several differences that reflect the underlying architecture and calling conventions.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In the differences you see are expected and necessary for the same C++ code to run efficiently and correctly on two different CPU architectures. The compiler generates code tailored to the capabilities and requirements of each target platform.

A couple of key differences include:
1. Register Size and Usage
•	x64 uses 64-bit registers (rax, rbx, rcx, rdx, etc.) and can address more memory directly.
•	x86 uses 32-bit registers (eax, ebx, ecx, edx, etc.).
•	In your x64 code, you see instructions like mov rdx, qword ptr [...] and add qword ptr [...], while in x86, it’s mov eax, dword ptr [...] and add dword ptr [...].

2. Calling Conventions
•	x64 Windows uses a different calling convention: the first four integer or pointer arguments are passed in registers (rcx, rdx, r8, r9), while x86 typically uses the stack or specific registers like ecx and edx.
•	This affects how function calls and returns are handled in the disassembly.

3. Instruction Set Differences
•	Some instructions are only available or more efficient in x64 (e.g., cmovg with 64-bit operands).
•	Memory addressing and stack management differ due to the larger address space in x64.

4. Data Model Differences
•	In x64, pointers and size_t are 8 bytes; in x86, they are 4 bytes. This changes how arrays, vectors, and memory offsets are handled.
•	The compiler may generate different code for the same C++ source to accommodate these differences

5. Optimization Differences
•	The compiler may choose different optimizations for each architecture, such as inlining, register allocation, or loop unrolling, based on the available resources and instruction set.

**Questions:**

*Is there anything you would like to ask?*
No.

### Final Reflection
At the end of this lab session I learnt about how to see the dissambley, as well as reading other debug windows, and the use of certain techniques to optimize code, such as breaking out of loops early and reducing redundant calculations. I also learnt about the differences in assembly code generated for x86 and x64 architectures. Overall, this lab session has enhanced my understanding of low-level programming concepts and performance optimization techniques.

---

## Lab D
### Week 4 - Lab D

24 Oct 2025

#### Introduction
This lab introduces you to debugging, as well as exploring some of the language constructs you have covered in recent lectures e.g. I/O, bitwise operators and assembly.

#### Q1. Object Parser

**Question:**

Open the Object Parser project.

Examine the partially completed parser and walk through the code.

The program should parse an OBJ file containing a description of objects. An example of which is sample.obj, which is located in Resource Files in the Solution Explorer.

The obj file contains a sequence of objects, where each object is delimited by white spaces.

Each object is defined by a set of vertices, textures, and normals.

The file format is as follows:

```c++
# comments

o object_name

v x y z

vt u v

vn x y z

f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
```

where
- o is the name of the object
- v is a vertex
- vt is a texture
- vn is a normal
- f is a face

The requirements of the completed project are:
1. Read the obj file and parse the objects
2. Ignore comments and any other lines that do not match the format.
3. Verify that the data is correctly parsed by printing the contents of the arrays.
4. The program should be able to parse multiple objects in the file.

The TODO statements show where to add the new code.

> Hint: All parsing can be accomplished with the stream `operator>>`

**Solution:**

```c++
int main(int argc, char** argv) {
	
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <input filename> <output filename>" << std::endl;
		return -1;
	}

	std::ifstream fin(argv[1]);
	if (!fin) {
		std::cerr << "Error: Failed to open file " << argv[1] << " for reading" << std::endl;
		return -1;
	}

	std::ofstream fout(argv[2]);
	if (!fout) {
		std::cerr << "Error: Failed to open file " << argv[2] << " for writing" << std::endl;
		return -1;
	}

	std::vector<std::string> objectNames;
	std::vector<Vertex> vertices;
	std::vector<Texture> textures;
	std::vector<Normal> normals;
	std::vector<int> vertexIndices;
	std::vector<int> textureIndices;
	std::vector<int> normalIndices;

	std::string tag;

    while (fin >> tag) {

        if (tag == "#") {
            // comment: skip rest of line            
            std::string throwaway;
            std::getline(fin, throwaway);
        }
        else if (tag == "o") {
            // object name
            std::string name;
            fin >> name;
            objectNames.push_back(name);
        }
        else if (tag == "v") {
            // vertex position
            Vertex vtx;
            fin >> vtx.x >> vtx.y >> vtx.z;
            vertices.push_back(vtx);
        }
        else if (tag == "vt") {
            // texture coord
            Texture tex;
            fin >> tex.u >> tex.v;
            textures.push_back(tex);
        }
        else if (tag == "vn") {
            // normal
            Normal n;
            fin >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (tag == "f") {
            // face: expect 3 tokens like "v/t/n"
            std::string t1, t2, t3;
            fin >> t1 >> t2 >> t3;
            auto parseFaceToken = [](const std::string& tok, int& vi, int& ti, int& ni) {
                // find slashes
                size_t s1 = tok.find('/');
                size_t s2 = tok.find('/', s1 + 1);

                // substrings to int
                vi = std::stoi(tok.substr(0, s1));
                ti = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1));
                ni = std::stoi(tok.substr(s2 + 1));
                };

            int vi, ti, ni;

            parseFaceToken(t1, vi, ti, ni);
            vertexIndices.push_back(vi);
            textureIndices.push_back(ti);
            normalIndices.push_back(ni);

            parseFaceToken(t2, vi, ti, ni);
            vertexIndices.push_back(vi);
            textureIndices.push_back(ti);
            normalIndices.push_back(ni);

            parseFaceToken(t3, vi, ti, ni);
            vertexIndices.push_back(vi);
            textureIndices.push_back(ti);
            normalIndices.push_back(ni);
        }
        else {
            // Unrecognised line start, skip rest of the line
            std::string throwaway;
            std::getline(fin, throwaway);
        }
    }

    fout << "Vertices:\n";
    for (const auto& v : vertices)
        fout << v.x << " " << v.y << " " << v.z << "\n";

    fout << "\nTextures:\n";
    for (const auto& t : textures)
        fout << t.u << " " << t.v << "\n";

    fout << "\nNormals:\n";
    for (const auto& n : normals)
        fout << n.x << " " << n.y << " " << n.z << "\n";

    fout << "\nFaces (vertex/texture/normal indices):\n";
    for (size_t i = 0; i < vertexIndices.size(); i += 3) {
        fout << vertexIndices[i] << "/" << textureIndices[i] << "/" << normalIndices[i] << " ";
        fout << vertexIndices[i + 1] << "/" << textureIndices[i + 1] << "/" << normalIndices[i + 1] << " ";
        fout << vertexIndices[i + 2] << "/" << textureIndices[i + 2] << "/" << normalIndices[i + 2] << "\n";
    }

    return 0;
}
```


**Sample input:**

```
sample.obj
```

**Sample output:**

```
Vertices:
1 1 -1
1 -1 -1
1 1 1
1 -1 1
-1 1 -1
-1 -1 -1
-1 1 1
-1 -1 1

Textures:
0.625 0.5
0.875 0.5
0.875 0.75
0.625 0.75
0.375 0.75
0.625 1
0.375 1
0.375 0
0.625 0
0.625 0.25
0.375 0.25
0.125 0.5
0.375 0.5
0.125 0.75

Normals:
0 1 0
0 0 1
-1 0 0
0 -1 0
1 0 0
0 0 -1

Faces (vertex/texture/normal indices):
1/1/1 5/2/1 7/3/1
4/5/2 3/4/2 7/6/2
8/8/3 7/9/3 5/10/3
6/12/4 2/13/4 4/5/4
2/13/5 1/1/5 3/4/5
6/11/6 5/10/6 1/1/6

```


**Reflection:**
*Reflect on what you have learnt from this exercise.*
I learnt of techniques to parse .obj files as long as their structure is known as in what to read from the file. 

*Did you make any mistakes?* 
Yes, initially i tried to just build the project but there was a LINKER problem, I had to do some research and get help from a teacher in order to fix it. I got th

*In what way has your knowledge improved?*
In that now I know how to parse .obj files and how to handle file I/O in C++. And I also know how toe xecute a .exe file with command line arguments from Visual Studio and from the terminal.

**Questions:**
*Is there anything you would like to ask?*
No.


#### Q2. Object Parser (multiple objects)

**Question:**

This exercise is a continuation of the previous parser exercise, above. Be sure that the previous exercise is completed.

Now expand the project to allow multiple objects to be parsed.

The challenge is to be able to hold the data for multiple objects at the same time.

A test file `cubes.obj` in the project folder contains two cubes.

If you're feeling adventurous try creating some primitives in Blender, export as an OBJ file and load into your program.

**Solution:**
First I created a new struct for holding the data of each object:

```c++
struct ObjectData {
    std::string objectName;
    std::vector<Vertex> vertices;
    std::vector<Texture> textures;
    std::vector<Normal> normals;
    std::vector<int> vertexIndices;
    std::vector<int> textureIndices;
    std::vector<int> normalIndices;
};
```

Then I modified the main function to use a vector of ObjectData to hold multiple objects:


```c++
int main(int argc, char** argv) {
	
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <input filename> <output filename>" << std::endl;
		return -1;
	}

	std::ifstream fin(argv[1]);
	if (!fin) {
		std::cerr << "Error: Failed to open file " << argv[1] << " for reading" << std::endl;
		return -1;
	}

	std::ofstream fout(argv[2]);
	if (!fout) {
		std::cerr << "Error: Failed to open file " << argv[2] << " for writing" << std::endl;
		return -1;
	}

	std::vector<ObjectData> objects;
	ObjectData currentObject;

	bool hasObject = false;

	std::string tag;

    while (fin >> tag) {

        if (tag == "#") {
            // comment: skip rest of line            
            std::string throwaway;
            std::getline(fin, throwaway);
        }
        else if (tag == "o") {
            // object name
            if (hasObject) {
                objects.push_back(currentObject);
                currentObject = ObjectData();
            }
            fin >> currentObject.objectName;
            hasObject = true;
        }
        else if (tag == "v") {
            // vertex position
            Vertex vtx;
            fin >> vtx.x >> vtx.y >> vtx.z;
            currentObject.vertices.push_back(vtx);
        }
        else if (tag == "vt") {
            // texture coord
            Texture tex;
            fin >> tex.u >> tex.v;
            currentObject.textures.push_back(tex);
        }
        else if (tag == "vn") {
            // normal
            Normal n;
            fin >> n.x >> n.y >> n.z;
            currentObject.normals.push_back(n);
        }
        else if (tag == "f") {
            // face: expect 3 tokens like "v/t/n"
            std::string t1, t2, t3;
            fin >> t1 >> t2 >> t3;
            auto parseFaceToken = [](const std::string& tok, int& vi, int& ti, int& ni) {
                // find slashes
                size_t s1 = tok.find('/');
                size_t s2 = tok.find('/', s1 + 1);

                // substrings to int
                vi = std::stoi(tok.substr(0, s1));
                ti = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1));
                ni = std::stoi(tok.substr(s2 + 1));
            };

            int vi, ti, ni;

            parseFaceToken(t1, vi, ti, ni);
            currentObject.vertexIndices.push_back(vi);
            currentObject.textureIndices.push_back(ti);
            currentObject.normalIndices.push_back(ni);

            parseFaceToken(t2, vi, ti, ni);
            currentObject.vertexIndices.push_back(vi);
            currentObject.textureIndices.push_back(ti);
            currentObject.normalIndices.push_back(ni);

            parseFaceToken(t3, vi, ti, ni);
            currentObject.vertexIndices.push_back(vi);
            currentObject.textureIndices.push_back(ti);
            currentObject.normalIndices.push_back(ni);
        }
        else {
            // Unrecognised line start, skip rest of the line
            std::string throwaway;
            std::getline(fin, throwaway);
        }
    }

    if (hasObject) {
        objects.push_back(currentObject);
    }

    for(const auto& obj : objects) {
        fout << "Object: " << obj.objectName << "\n";
        fout << "Vertices:\n";
        for (const auto& v : obj.vertices)
            fout << v.x << " " << v.y << " " << v.z << "\n";

        fout << "\nTextures:\n";
        for (const auto& t : obj.textures)
            fout << t.u << " " << t.v << "\n";

        fout << "\nNormals:\n";
        for (const auto& n : obj.normals)
            fout << n.x << " " << n.y << " " << n.z << "\n";

        fout << "\nFaces (vertex/texture/normal indices):\n";
        for (size_t i = 0; i < obj.vertexIndices.size(); i += 3) {
            fout << obj.vertexIndices[i] << "/" << obj.textureIndices[i] << "/" << obj.normalIndices[i] << " ";
            fout << obj.vertexIndices[i + 1] << "/" << obj.textureIndices[i + 1] << "/" << obj.normalIndices[i + 1] << " ";
            fout << obj.vertexIndices[i + 2] << "/" << obj.textureIndices[i + 2] << "/" << obj.normalIndices[i + 2] << "\n";
        }

        fout << "\n" << "------------------------------------" << "\n";
    }

    return 0;
}
```


**Sample Input:**
```
cubes.obj
```


**Sample output:**

```
Object: Cube
Vertices:
1 1 -1
1 -1 -1
1 1 1
1 -1 1
-1 1 -1
-1 -1 -1
-1 1 1
-1 -1 1

Textures:
0.625 0.5
0.875 0.5
0.875 0.75
0.625 0.75
0.375 0.75
0.625 1
0.375 1
0.375 0
0.625 0
0.625 0.25
0.375 0.25
0.125 0.5
0.375 0.5
0.125 0.75

Normals:
0 1 0
0 0 1
-1 0 0
0 -1 0
1 0 0
0 0 -1

Faces (vertex/texture/normal indices):
1/1/1 5/2/1 7/3/1
4/5/2 3/4/2 7/6/2
8/8/3 7/9/3 5/10/3
6/12/4 2/13/4 4/5/4
2/13/5 1/1/5 3/4/5
6/11/6 5/10/6 1/1/6

------------------------------------
Object: Cube.001
Vertices:
2.27227 0 4.44561
2.27227 0.570662 4.44561
2.27227 0 1.74199
2.27227 0.570662 1.74199
5.61656 0 4.44561
5.61656 0.570662 4.44561
5.61656 0 1.74199
5.61656 0.570662 1.74199

Textures:
0.375 0
0.625 0
0.625 0.25
0.375 0.25
0.625 0.5
0.375 0.5
0.625 0.75
0.375 0.75
0.625 1
0.375 1
0.125 0.5
0.125 0.75
0.875 0.5
0.875 0.75

Normals:
-1 0 0
0 0 -1
1 0 0
0 0 1
0 -1 0
0 1 0

Faces (vertex/texture/normal indices):
9/15/7 10/16/7 12/17/7
11/18/8 12/17/8 16/19/8
15/20/9 16/19/9 14/21/9
13/22/10 14/21/10 10/23/10
11/25/11 15/20/11 13/22/11
16/19/12 12/27/12 10/28/12

------------------------------------

```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt how to manage multiple objects in a single file by creating a struct to hold each object's data and using a vector to store multiple instances of that struct.


*Did you make any mistakes?* 
Not really, just had to do some adjustments to the previous code to accommodate multiple objects and print something readable.

*In what way has your knowledge improved?*
In that holding structs and abstracting data into manageable pieces is very useful when dealing with multiple entities.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q3. Tuples

**Question:**

Now for some C++ 17 syntax.

Select one of your object parsers and wrap the code within a new function.  This new function has a single parameter, the file name, and returns a `tuple` consisting of the size of the largest object and the level of the largest object.

**Solution:**

I had to change the Language in the project properties for the C++ 17 and then I created a new function `parseObjFile` that takes the filename as a parameter and returns a tuple containing the name, size, and level of the largest object:

```c++
std::tuple<std::string, int, int> parseObjFile(std::vector<ObjectData>& objects) {
    // Find the largest object by number of vertices
    std::string name = "";
    int maxSize = 0;
    int maxLevel = -1;
    for (size_t i = 0; i < objects.size(); ++i) {
        if (static_cast<int>(objects[i].vertices.size()) > maxSize) {
			name = objects[i].objectName;
            maxSize = static_cast<int>(objects[i].vertices.size());
            maxLevel = static_cast<int>(i);
        }
    }
    return { name, maxSize, maxLevel };
}
```

the  I called it in the main after looping through the objects:

```c++
    auto [name, largestSize, largestLevel] = parseObjFile(objects);
    std::cout << "Largest object name: " << name << ", Largest object size : " << largestSize << ", Level : " << largestLevel << std::endl;
```


**Sample input:**

```
cubes.obj
```

**Sample output:**
```
Largest object name: Cube, Largest object size : 8, Level : 0
```


**Reflection:**

*Reflect on what you have learnt from this exercise.*
I created a function that returns multiple values using a tuple, which is a useful feature in C++17 for returning multiple related values from a function. And thanks to tuples I can unpack the returned values easily.

*Did you make any mistakes?*
No, as far as I can tell, the implementation went smoothly.


*In what way has your knowledge improved?*
In that I now understand how to use tuples in C++17 to return multiple values from a function and how to unpack them conveniently.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q4. Span and Arrays

**Question:**

Open the **Arrays** project.

The code allocates an array of 1000 values, assigns random values to this array, and then calls a function to determine the largest value in this array.

Create a second version that uses the `array` template, which wraps a vanilla C array within a C++11 template.

Create a third version of this function that uses the C++20 `span` to pass the array to the function.  `span` removes the need for the second parameter.

>Note: `span` is C++20 language feature that requires a change to the project settings:  C/C++ -> Language -> C++ Language Support -> ISO C++20 Standard (/std:c++20)

**Solution:**

First the original version:
```c++
#include <array>

template <std::size_t N>
int findLargestValueV2(const std::array<int, N>& listOfValues) {
	if (listOfValues.empty())
		return std::numeric_limits<int>::min();
	auto largestValue = listOfValues[0];
	for (std::size_t i = 1; i < N; ++i) {
		if (listOfValues[i] > largestValue)
			largestValue = listOfValues[i];
	}
	return largestValue;
}
```

Then the second version that uses the array template, which wraps a vanilla C array within a C++11 template:
```c++
int findLargestValueV1(const int* listOfValues, const unsigned int numOfValues) {

	if (numOfValues == 0)
		return std::numeric_limits<int>::min();

	auto largestValue = listOfValues[0];
	for (auto i = 1u; i < numOfValues; i++) {
		if (listOfValues[i] > largestValue)
			largestValue = listOfValues[i];
	}
	return largestValue;
}
```

Finaly, the third version of this function that uses the C++20 span to pass the array to the function. span removes the need for the second parameter:
```c++
#include <span>

int findLargestValueV3(std::span<const int> listOfValues) {
	if (listOfValues.empty())
		return std::numeric_limits<int>::min();
	auto largestValue = listOfValues[0];
	for (size_t i = 1; i < listOfValues.size(); ++i) {
		largestValue = (listOfValues[i] > largestValue) ? listOfValues[i] : largestValue;
	}
	return largestValue;
}
```


**Sample input:**

```
an array of 1000 values that are assigned random values
```

**Sample output:**

```
V1 - Largest value = 32638
V2 - Largest value = 32638
V3 - Largest value = 32638

```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I changed the original function to use std::array and std::span, which are modern C++ features that provide better safety and convenience when working with arrays.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In the different ways to handle arrays in modern C++, and the benefits of using std::array and std::span for better type safety and easier management of array sizes.

**Questions:**

*Is there anything you would like to ask?*
No.

### Final Reflection
At the end of this exercise I started to see how this module starts to interconnect with other modules such as 3D Modelling and Game Engines as .obj files are used in both of those modules. And how to properly parse them is important for loading 3D models into a game engine.
I learnt about tuples and spans which are useful features in modern C++ for returning multiple values from functions and for safer array handling respectively. And about the different ways to handle arrays in C++ depending on the use case and the lmitations of the language versions.

---

## Lab E
### Week 5 - Lab E

31 Oct 2025

#### Introduction
This lab introduces you to debugging, as well as exploring some of the language constructs you have covered in recent lectures e.g. I/O, bitwise operators and assembly.

#### Q1. Basic vectors

**Question:**
Open the project **Vector Basics**. This project contains a class `Vector3d`. Walk through the code for `Vector3d` and familiarize yourself with the names and structure of the methods.

Why are some methods in-lined and others not?

Add the following new methods:

- Vector product (i.e. cross product)
- Scalar product (i.e. dot product)

- Add new methods which overload the following binary operators:

- `+` vector addition
- `-` vector subtraction
- `*` scalar product
- `^` vector product
- `+=` vector addition
- `-=` vector subtraction
- `<<` stream out
- `>>` stream in

Add new methods which overload the following unary operators:

- `-` vector inversion (reverse the vector)

Now that you have 3 methods to add two vectors, use the timing code from early labs to analyse the performance of each implementation.

**Solution:**

- For `+` operator:
```c++
// BEGIN payload

// Test addition of two Vector3d objects using operator +
Vector3d a(1.0, 2.0, 3.0);
Vector3d b(4.0, 5.0, 6.0);
Vector3d c = a + b;

// Prevent optimizer from removing the code
dummyX += c._x + c._y + c._z;

// END payload
```

- For `+=` operator:
```c++
// BEGIN payload

// Test addition of two Vector3d objects using operator +=
Vector3d a(1.0, 2.0, 3.0);
Vector3d b(4.0, 5.0, 6.0);
a += b; // a is now (5.0, 7.0, 9.0)

// Prevent optimizer from removing the code
dummyX += a._x + a._y + a._z;

// END payload
```

- For the add function:
```c++
// BEGIN payload

// Test addition of two Vector3d objects using the add() method
Vector3d a(1.0, 2.0, 3.0);
Vector3d b(4.0, 5.0, 6.0);
Vector3d c = a.add(b);

// Prevent optimizer from removing the code
dummyX += c._x + c._y + c._z;

// END payload
```
**Sample input:**
```c++
Vector3d a(1.0, 2.0, 3.0);
Vector3d b(4.0, 5.0, 6.0);
```

**Sample output:**
For operator + timing:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.57075

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
26 26 26 26 26 26 26 26 26 28 28 28 28 28 28 28 28 28 28 30 30 30 30 30 30 32 32 32 32 32 34 34 34 36 36 38 38 40 40 40 42 42 44 46 48 48 48 48 50 52 52 54 54 54 56 56 58 58 58 58 58 60 60 60 60 62 66 66 66 68 68 70 70 76 76 86 94 100 108 108 110 124 132 132 132 138 152 152 166 168 206 232 246 288 908 14486 17086 34638 35304 85396
........................

x= 5.25e+06 (dummy value - ignore)
```

For `+=` operator:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.63071

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
30 30 30 30 30 30 30 30 30 30 32 32 32 32 32 34 34 34 34 34 36 36 36 38 40 42 42 42 44 44 44 44 46 48 48 48 50 50 52 54 54 54 54 56 56 58 58 58 60 60 62 62 62 62 62 62 62 62 62 62 62 64 64 66 66 66 66 68 68 72 76 76 78 78 84 88 94 98 108 110 110 122 126 128 130 136 146 150 162 164 164 188 218 238 240 240 21048 22546 23812 26270
........................

x= 5.25e+06 (dummy value - ignore)
```

- For the add function:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.58363

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
24 24 26 26 26 26 26 26 26 26 26 26 28 28 28 28 28 30 30 30 30 30 30 30 30 32 34 34 36 38 40 40 40 40 42 42 44 44 46 46 46 48 48 50 50 52 52 52 52 52 52 54 54 56 56 56 56 58 58 58 58 60 60 60 60 62 62 62 62 64 64 66 68 70 70 76 76 82 88 90 96 106 124 130 136 148 154 156 158 168 170 174 242 248 286 292 316 13918 18146 31564
........................

x= 5.25e+06 (dummy value - ignore)
```
**Reflection:**
*Reflect on what you have learnt from this exercise.*
About the question `Why are some methods in-lined and others not?` is due the fact that inlined methods are usually small and simple, allowing the compiler to replace the method call with the actual code, which can improve performance by reducing function call overhead. Non-inlined methods are typically larger or more complex, where inlining could lead to code bloat and reduced readability.

*Did you make any mistakes?* 
Just for the adding of the masm.asm I need to ask for help to the tutor.

*In what way has your knowledge improved?*
In that I have learned how to implement vector operations and overload operators in C++, as well as understanding the performance implications of different implementations.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q2. Commutativity


**Question:**
Within vector mathematics it is possible to multiply a vector by a single number.

Implement both a standard method and overload the `*` operator to multiply a vector by a single double.

Also implement the multiplication of a single double by a vector.

Why is this last requirement more problematic than the preceding requirement?

**Solution:**
```c++
// Multiply vector by scalar (standard method)
Vector3d multiply(const double scalar) const {
	return Vector3d(_x * scalar, _y * scalar, _z * scalar);
}

// Multiply vector by scalar (operator overload)
Vector3d operator*(const double scalar) const {
	return Vector3d(_x * scalar, _y * scalar, _z * scalar);
}

// Multiply scalar by vector (operator overload, non-member)
friend Vector3d operator*(const double scalar, const Vector3d& v) {
	return Vector3d(v._x * scalar, v._y * scalar, v._z * scalar);
}
```

**Sample input:**


**Sample output:**

**Reflection:**
*Reflect on what you have learnt from this exercise.*
Answering the question `Why is this last requirement more problematic than the preceding requirement?` is because in C++, operator overloading is done within the class of the left-hand operand. When multiplying a vector by a scalar (vector * scalar), the operator can be defined within the Vector3d class. However, when multiplying a scalar by a vector (scalar * vector), the left-hand operand is not of type Vector3d, so the operator cannot be defined within that class. This requires defining a non-member function to handle this case, which can be less intuitive and requires careful consideration of namespace and access to private members.

*Did you make any mistakes?* 
No.

*In what way has your knowledge improved?*
I have learned about the limitations of operator overloading in C++ and how to implement non-member functions to handle cases where the left-hand operand is not of the class type.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q3. Matrices

**Question:**
Open the project Matrices. This project contains a partial implementation of the class `Matrix33d`.

Walk through the code for `Matrix33d` and familiarise yourself with the names and structure of the methods.

Using the knowledge gained from the previous exercises on Vector mathematics, complete the `Matrxix33d` class.

Functionality to be included:

- Addition
- Subtraction
- Multiplication
- Streaming in and out
- Inverse
- Transpose

Your implementation should balance efficiency with maintainability.

**Solution:**
For the header:
```c++
#pragma once

#include "Vector3d.h"
#include <iostream>

class Matrix33d {
    static constexpr double _epsilon = 1.0e-8;
    Vector3d _row[3]{};

public:
    Matrix33d() = default;

    explicit Matrix33d(const double m[9]) {
        _row[0] = Vector3d(m[0], m[1], m[2]);
        _row[1] = Vector3d(m[3], m[4], m[5]);
        _row[2] = Vector3d(m[6], m[7], m[8]);
    }

    // Addition
    Matrix33d operator+(const Matrix33d& rhs) const {
        Matrix33d result;
        for (int i = 0; i < 3; ++i)
            result._row[i] = _row[i] + rhs._row[i];
        return result;
    }

    // Subtraction
    Matrix33d operator-(const Matrix33d& rhs) const {
        Matrix33d result;
        for (int i = 0; i < 3; ++i)
            result._row[i] = _row[i] - rhs._row[i];
        return result;
    }

    // Multiplication (Matrix * Matrix)
    Matrix33d operator*(const Matrix33d& rhs) const {
        Matrix33d result;
        for (int i = 0; i < 3; ++i) {
            result._row[i]._x = _row[i].dot(Vector3d(rhs._row[0]._x, rhs._row[1]._x, rhs._row[2]._x));
            result._row[i]._y = _row[i].dot(Vector3d(rhs._row[0]._y, rhs._row[1]._y, rhs._row[2]._y));
            result._row[#pragma once

#include "Vector3d.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

class Matrix33d {
    static constexpr double _epsilon = 1.0e-8;
    Vector3d _row[3];

public:
    Matrix33d() : _row{ Vector3d(), Vector3d(), Vector3d() } {}

    Matrix33d(const double m[9]) {
        _row[0] = Vector3d(m[0], m[1], m[2]);
        _row[1] = Vector3d(m[3], m[4], m[5]);
        _row[2] = Vector3d(m[6], m[7], m[8]);
    }

    Matrix33d(const Vector3d& r0, const Vector3d& r1, const Vector3d& r2) {
        _row[0] = r0;
        _row[1] = r1;
        _row[2] = r2;
    }

    // Addition
    Matrix33d operator+(const Matrix33d& rhs) const {
        return Matrix33d(_row[0] + rhs._row[0], _row[1] + rhs._row[1], _row[2] + rhs._row[2]);
    }

    // Subtraction
    Matrix33d operator-(const Matrix33d& rhs) const {
        return Matrix33d(_row[0] - rhs._row[0], _row[1] - rhs._row[1], _row[2] - rhs._row[2]);
    }

    // Multiplication (Matrix * Matrix)
    Matrix33d operator*(const Matrix33d& rhs) const {
        Matrix33d result;
        for (int i = 0; i < 3; ++i) {
            result._row[i]._x = _row[i]._x * rhs._row[0]._x + _row[i]._y * rhs._row[1]._x + _row[i]._z * rhs._row[2]._x;
            result._row[i]._y = _row[i]._x * rhs._row[0]._y + _row[i]._y * rhs._row[1]._y + _row[i]._z * rhs._row[2]._y;
            result._row[i]._z = _row[i]._x * rhs._row[0]._z + _row[i]._y * rhs._row[1]._z + _row[i]._z * rhs._row[2]._z;
        }
        return result;
    }

    // Transpose
    Matrix33d transpose() const {
        return Matrix33d(
            Vector3d(_row[0]._x, _row[1]._x, _row[2]._x),
            Vector3d(_row[0]._y, _row[1]._y, _row[2]._y),
            Vector3d(_row[0]._z, _row[1]._z, _row[2]._z)
        );
    }

    // Inverse (implemented in cpp)
    Matrix33d inverse() const;

    // Streaming out
    friend std::ostream& operator<<(std::ostream& os, const Matrix33d& m) {
        for (int i = 0; i < 3; ++i)
            os << m._row[i]._x << " " << m._row[i]._y << " " << m._row[i]._z << std::endl;
        return os;
    }

    // Streaming in
    friend std::istream& operator>>(std::istream& is, Matrix33d& m) {
        for (int i = 0; i < 3; ++i)
            is >> m._row[i]._x >> m._row[i]._y >> m._row[i]._z;
        return is;
    }

    // Access row
    const Vector3d& operator[](int i) const { return _row[i]; }
    Vector3d& operator[](int i) { return _row[i]; }
};
```

For the body:
```c++
#include "Matrix33d.h"

Matrix33d Matrix33d::inverse() const {
    double det =
        _row[0]._x * (_row[1]._y * _row[2]._z - _row[1]._z * _row[2]._y) -
        _row[0]._y * (_row[1]._x * _row[2]._z - _row[1]._z * _row[2]._x) +
        _row[0]._z * (_row[1]._x * _row[2]._y - _row[1]._y * _row[2]._x);

    if (std::abs(det) < _epsilon)
        throw std::runtime_error("Matrix33d::inverse: Matrix is singular or nearly singular.");

    Matrix33d inv;
    inv._row[0]._x = (_row[1]._y * _row[2]._z - _row[1]._z * _row[2]._y) / det;
    inv._row[0]._y = -(_row[0]._y * _row[2]._z - _row[0]._z * _row[2]._y) / det;
    inv._row[0]._z = (_row[0]._y * _row[1]._z - _row[0]._z * _row[1]._y) / det;

    inv._row[1]._x = -(_row[1]._x * _row[2]._z - _row[1]._z * _row[2]._x) / det;
    inv._row[1]._y = (_row[0]._x * _row[2]._z - _row[0]._z * _row[2]._x) / det;
    inv._row[1]._z = -(_row[0]._x * _row[1]._z - _row[0]._z * _row[1]._x) / det;

    inv._row[2]._x = (_row[1]._x * _row[2]._y - _row[1]._y * _row[2]._x) / det;
    inv._row[2]._y = -(_row[0]._x * _row[2]._y - _row[0]._y * _row[2]._x) / det;
    inv._row[2]._z = (_row[0]._x * _row[1]._y - _row[0]._y * _row[1]._x) / det;

    return inv;
}
```

**Reflection:**

*Reflect on what you have learnt from this exercise.* I learnt how to implement matrix operations such as addition, subtraction and multiplication, as well as how to handle matrix inversion and transposition. I also gained experience in operator overloading for custom classes in C++.


*Did you make any mistakes?*

*In what way has your knowledge improved?*
In that I have learned how to implement matrix operations and overload operators in C++, as well as understanding the mathematical concepts behind these operations.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q4. Vector and Matrix Multiplication
**Question:**
Expand your Matrix33d class to be able to multiple a Vector3d object by a Matrix33d object.

**Solution:**

In the Matrix33d header file, I added the following friend function:
```c++
    // Multiply Vector3d by Matrix33d (row vector * matrix)
    friend Vector3d operator*(const Vector3d& v, const Matrix33d& m) {
        return Vector3d(
            v._x * m._row[0]._x + v._y * m._row[1]._x + v._z * m._row[2]._x,
            v._x * m._row[0]._y + v._y * m._row[1]._y + v._z * m._row[2]._y,
            v._x * m._row[0]._z + v._y * m._row[1]._z + v._z * m._row[2]._z
        );
    }
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Why a friend function? 

- Access: It can access private members of Matrix33d directly, making the implementation simple and efficient.
- Operator Overloading: In C++, operator overloading for cases where the left-hand operand is not the class type (e.g., Vector3d * Matrix33d) must be done as a non-member function. Declaring it as a friend allows access to internals without exposing them publicly.

*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In that I have learned how to implement multiplication between different classes and the use of friend functions in C++.

**Questions:**

*Is there anything you would like to ask?*

#### Q5. Internal data structures
**Question:**
Now that you are familiar with the two classes, examine the internal data structures.

Is having the components of a vector stored as individual attributes a good implementation, or would it be advantageous to instead store the components in an array of rank 1 and size 3?

The current `Matrxi33d` is implemented using `Vector3ds`. Is this a good approach, or would it be better to implement as either an array of rank 2 and size 3 or an array of rank 1 and size 9?

Now implement the `Matrxi33d` using one of these different data formats and assess the performance using the timing code from earlier labs.

**Solution:**
I created two strcuts for matrices with different internal data structures and compared their performance in matrix addition:
```c++
		// --- Matrix33dA: Uses Vector3d _row[3] ---
		struct Matrix33dA {
			Vector3d _row[3];
			Matrix33dA() : _row{ Vector3d(), Vector3d(), Vector3d() } {}
			Matrix33dA(const double m[9]) {
				_row[0] = Vector3d(m[0], m[1], m[2]);
				_row[1] = Vector3d(m[3], m[4], m[5]);
				_row[2] = Vector3d(m[6], m[7], m[8]);
			}
			Matrix33dA operator+(const Matrix33dA& rhs) const {
				double m[9] = {
					_row[0]._x + rhs._row[0]._x, _row[0]._y + rhs._row[0]._y, _row[0]._z + rhs._row[0]._z,
					_row[1]._x + rhs._row[1]._x, _row[1]._y + rhs._row[1]._y, _row[1]._z + rhs._row[1]._z,
					_row[2]._x + rhs._row[2]._x, _row[2]._y + rhs._row[2]._y, _row[2]._z + rhs._row[2]._z
				};
				return Matrix33dA(m);
			}
		};

		// --- Matrix33dB: Uses double _m[9] ---
		struct Matrix33dB {
			double _m[9];
			Matrix33dB() { for (int i = 0; i < 9; ++i) _m[i] = 0.0; }
			Matrix33dB(const double m[9]) { for (int i = 0; i < 9; ++i) _m[i] = m[i]; }
			Matrix33dB operator+(const Matrix33dB& rhs) const {
				Matrix33dB result;
				for (int i = 0; i < 9; ++i) result._m[i] = _m[i] + rhs._m[i];
				return result;
			}
		};
```

For the Matrix33dA implementation:
```c++
// BEGIN payload

// --- Test Matrix Addition for Both Implementations ---
Matrix33dA aA(m1), bA(m2);
Matrix33dA cA = aA + bA;

// Prevent optimizer from removing the code
dummyX += cA._row[0]._x + cA._row[1]._y + cA._row[2]._z;

// END payload
```

For the Matrix33dB implementation:
```c++
// BEGIN payload

// --- Test Matrix Addition for Both Implementations ---
Matrix33dB aB(m1), bB(m2);
Matrix33dB cB = aB + bB;

// Prevent optimizer from removing the code
dummyX += cB._m[0] + cB._m[4] + cB._m[8];

// END payload
```

**Sample input:**

```c++
// --- Test Data ---
double m1[9] = { 1,2,3,4,5,6,7,8,9 };
double m2[9] = { 9,8,7,6,5,4,3,2,1 };
```

**Sample output:**

- For the Matrix33dA implementation:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 1.69972

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
42 42 42 42 42 42 42 44 44 44 44 46 52 52 54 54 54 54 54 56 56 56 56 56 56 56 56 58 60 60 60 60 62 62 62 62 66 66 66 68 68 68 70 70 72 76 76 76 82 82 86 86 86 96 108 110 122 126 126 126 128 128 130 130 130 130 130 132 132 132 132 134 134 136 136 138 138 140 140 140 154 156 156 164 166 168 170 170 174 226 232 242 254 282 290 300 384 20780 26112 31398
........................

x= 7.5e+06 (dummy value - ignore)
```

- For the Matrix33dB implementation:
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 2

Mean (80%) duration: 2.29494

Sample data [100]
........................
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
........................
38 38 38 38 38 38 38 38 38 40 40 40 40 40 40 40 40 40 40 40 40 42 42 42 42 42 42 42 42 42 44 44 44 44 44 44 44 44 46 46 46 46 46 46 48 48 48 48 48 48 50 50 50 50 50 52 52 52 54 56 56 58 58 58 60 60 62 62 62 64 64 66 68 70 80 80 84 86 88 92 94 98 100 126 142 148 150 168 178 178 184 190 250 260 394 486 12194 19016 21386 25058
........................

x= 7.5e+06 (dummy value - ignore)
```
**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt about different internal data structures for implementing matrices and their performance implications. Storing matrix elements in a flat array of doubles proved to be more efficient than using an array of Vector3d objects, likely due to better memory locality and reduced overhead from object management.


*Did you make any mistakes?*
No.

*In what way has your knowledge improved?*
In that I have learned about different internal data structures for implementing matrices and their performance implications. Using an array of doubles for the matrix elements proved to be more efficient than using an array of Vector3d objects, likely due to better memory locality and reduced overhead from object management.


**Questions:**

*Is there anything you would like to ask?*
No.


### Final Reflection
I learnt that Vector3d could be arrenged in-house by a developer and/or modyfied its functions and overloading the operators in order to attend exclusive needs. Funny thing, for example it was the operator +, for example the fastes one among the different takes on adding vectors. I also had to learn and use of friend functions, even thought is not recommended by design. The use of matrices is also a pillar in 3d computer graphics, learning how expensive and fast certain operations are is an valuable learning experience.

---

## Lab F
### Week 6 - Lab F

12 Nov 2025

#### Introduction
The aim of this lab is to become familiar with the gang of three and deep copying, as well as creating the basis for exploring the gang of five during a future lab.


#### Q1. Big strings

**Question:**
Open the project Big String. This project contains a partially implemented class BigString to efficiently handle large strings.

Expand this class to contain at least the following functionality:

- Constructors
- Destructor
- Assignment operator
- Stream in and out
- Index operator

Now instrument the code, by placing debug statements within each method which stream out the name of the method. e.g.

```c++
fout << "BigString( const& )" << std::endl;
```

> Hint: Intlist, introduced in the last lecture, should give you ideas on how to create the new class

**Solution:**

- For the header file:
```c++
#pragma once
#include <iostream>
#include <ostream>

class BigString {
	char* _arrayOfChars;
	int _size;
	
public:
	BigString();
	~BigString();

	BigString(const BigString& other);
	BigString& operator=(const BigString& other);

	char& operator[](int index);

	friend std::ostream& operator<<(std::ostream& os, const BigString& bs);
	friend std::istream& operator>>(std::istream& is, BigString& bs);
};
```

- For the body file:
```c++
#include "BigString.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <string.h>

// CONTRUCTOR
BigString::BigString() : _size(100) {
    std::cout << "BigString()" << std::endl;
    _arrayOfChars = new char[_size];
    _arrayOfChars[0] = '\0';
}

// DESTRUCTOR
BigString::~BigString() {
    std::cout << "~BigString()" << std::endl;
    delete[] _arrayOfChars;
}

// COPY CONSTRUCTOR
BigString::BigString(const BigString& other) : _size(other._size) {
    std::cout << "BigString(const BigString&)" << std::endl;
    _arrayOfChars = new char[_size];
    strcpy_s(_arrayOfChars, _size, other._arrayOfChars);
}

// COPY ASSIGNMENT OPERATOR
BigString& BigString::operator=(const BigString& other) {
    std::cout << "operator=(const BigString&)" << std::endl;
    if (this != &other) {
        delete[] _arrayOfChars;
        _size = other._size;
        _arrayOfChars = new char[_size];
        strcpy_s(_arrayOfChars, _size, other._arrayOfChars);
    }
    return *this;
}

// INDEX OPERATOR
char& BigString::operator[](int index) {
    std::cout << "operator[](" << index << ")" << std::endl;
    if (index < 0 || index >= _size) {
        throw std::out_of_range("Index out of range");
    }
    return _arrayOfChars[index];
}

// OUTPUT STREAM OPERATOR
std::ostream& operator<<(std::ostream& os, const BigString& bs) {
    os << bs._arrayOfChars;
    return os;
}


// INPUT STREAM OPERATOR
std::istream& operator>>(std::istream& is, BigString& bs) {
    char buffer[1024];
    is >> buffer;
    int len = static_cast<int>(std::strlen(buffer));
    if (len >= bs._size) {
        delete[] bs._arrayOfChars;
        bs._size = len + 1;
        bs._arrayOfChars = new char[bs._size];
    }
    strcpy_s(bs._arrayOfChars, bs._size, buffer);
    return is;
}
```
**Sample input:**
N/A

**Sample output:**
N/A

**Reflection:**
*Reflect on what you have learnt from this exercise.*
I learnt that when a class manages its own dynamic memory, like using `new[]` we need to ensure that the memory is allocated correctly, copied correctly, free correctly to avoid memory leaks and dangling pointers. Implementing the copy constructor and assignment operator ensures that each object has its own copy of the data, preventing unintended side effects when objects are copied or assigned.

*Did you make any mistakes?* 
No as far as I can tell.

*In what way has your knowledge improved?*
The proper use of the Gang of Three (constructor, destructor, copy constructor, and assignment operator) has improved my understanding of resource management in C++. I now have a better grasp of how to handle dynamic memory safely and effectively in custom classes.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q2. Test harness


**Question:**
Now that the BigString is complete, create code to test all the functionality within BigString. Include at least the following tests:

1. Pass BigString to a function, by value
2. Pass BigString to a function, by reference
3. Return BigString from a function, by value
4. Return BigString from a function, by reference
5. Assign one BigString object to another

Also check for possible memory leaks, using the mechanism discussed during the last lecture.

**Solution:**
- In the Source.cpp file:
```c++
#include <iostream>
#include "BigString.h"

// Pass by value
void testByValue(BigString bs) {
    std::cout << "testByValue()" << std::endl;
    std::cout << bs << std::endl;
}

// Pass by reference
void testByReference(BigString& bs) {
    std::cout << "testByReference()" << std::endl;
    std::cout << bs << std::endl;
}

// Return by value
BigString returnByValue() {
    std::cout << "returnByValue()" << std::endl;
    BigString temp;
    std::cin >> temp;
    return temp;
}

// Return by reference (not recommended for local objects, so use static for demonstration)
BigString& returnByReference() {
    std::cout << "returnByReference()" << std::endl;
    static BigString temp;
    std::cin >> temp;
    return temp;
}

int main() {
    std::cout << "Main start" << std::endl;

    BigString a;
    std::cout << "Enter string for a: ";
    std::cin >> a;

    // Assignment operator
    BigString b;
    b = a;
    std::cout << "After assignment, b: " << b << std::endl;

    // Pass by value
	testByValue(a); //NOTE: a is copied, this will invoke the copy constructor and the parameter bs in testByValue is a separate object and will be destroyed.

    // Pass by reference
	testByReference(a); // NOTE: a is not copied, no copy constructor is invoked.

    // Return by value
    std::cout << "Enter string for returnByValue: ";
    BigString c = returnByValue();
    std::cout << "Returned by value, c: " << c << std::endl;

    // Return by reference
    std::cout << "Enter string for returnByReference: ";
	BigString& d = returnByReference(); // NOTE: d is a reference to the static object inside returnByReference. Using static keeps the object alive after the function returns.
    std::cout << "Returned by reference, d: " << d << std::endl;

    // Index operator
    std::cout << "First character of a: " << a[0] << std::endl;

    std::cout << "\nMain end" << std::endl;
    return 0;
}
```

**Sample input:**
- For BigString a: HelloWorld
- For returnByValue: TestValue
- For returnByReference: TestReference

**Sample output:**

```
BigString()
operator=(const BigString&)
After assignment, b: HelloWorld
BigString(const BigString&)
testByValue()
HelloWorld
~BigString()
testByReference()
HelloWorld
Enter string for returnByValue: returnByValue()
BigString()
TestValue
Returned by value, c: TestValue
Enter string for returnByReference: returnByReference()
BigString()
TestReference
Returned by reference, d: TestReference
operator[](0)
First character of a: H

Main end
~BigString()
~BigString()
~BigString()
~BigString()
```

**Reflection:**
*Reflect on what you have learnt from this exercise.*
- All required scenarios are covered and will trigger the debug output.
- No memory leaks: every new in BigString is paired with a delete[] in the destructor, and assignment/copy is handled safely.
- The static object in returnByReference() is used only for demonstration; returning references to local objects is unsafe.

*Did you make any mistakes?* 
Not as far as I can tell.

*In what way has your knowledge improved?*
In that now I can see the outputs of the different function calls and how they interact with the BigString class, especially in terms of memory management and object lifecycle.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q3. Optimisation

**Question:**
Now that you have a clear picture of which BigString functions are being called; are there any situations where you think you can improve the performance of your code?

**Solution:**
1. Avoid Unnecessary Memory Allocations
- In the assignment operator and copy constructor, I always allocate new memory and copy the string, even if the source and destination are the same or if the string is empty.
- Sympthom: Unnecessary memory allocations can lead to performance degradation, especially with large strings. Every deep copy triggers:
    - A delete[]
    - A new[]
    - A strcpy_s
- Improvement: Check for self-assignment in the assignment operator (already done), but also consider using the copy-and-swap idiom for exception safety and efficiency. Copy-and-swap idiom, which:
    - Provides exception safety
    - Reduces code duplication
    - Can reuse existing memory in some designs
    - Is the standard best practice before move semantics existed
    - ```cpp
          void swap(BigString& other) noexcept {
            std::swap(_arrayOfChars, other._arrayOfChars);
            std::swap(_size, other._size);
          }

	      BigString& operator=(BigString other) {
	        swap(other);
	        return *this;
	      }
	    ```


2. Use Move Semantics
- The class currently does not implement a move constructor or move assignment operator. These are part of the "Rule of Five" and can greatly improve performance when objects are returned by value or moved.
- Improvement: Implement move constructor and move assignment operator to avoid unnecessary deep copies.
    - ```cpp
        BigString(BigString&& other) noexcept 
            : _arrayOfChars(other._arrayOfChars), _size(other._size) 
        {
            other._arrayOfChars = nullptr;
            other._size = 0;
        }

      ```

3. Use Standard Library Functions
- I use strcpy_s for copying strings, which is safe, but using std::string would eliminate manual memory management and reduce the risk of leaks and buffer overflows. Let's not reinvent the wheel.
- Improvement: If allowed, refactor to use std::string internally.


4. Exception Safety
- If memory allocation fails, the class may leak memory or leave objects in an invalid state.
- Improvement: Use **smart pointers** or the copy-and-swap idiom for better exception safety.

5. Stream Operators
- The stream operators use a fixed-size buffer for input, which may not be efficient for very large strings.
- Improvement: Read input in a loop or use stream iterators for more robust input handling.
    - Problems detected:
        - operator>> cannot read spaces
        - Large input can require repeated reallocation
        - I cannot append — only replace

- Possible improvements:
    - Use std::istream::getline
    - Implement dynamic growth strategies
    - Or accept an input iterator range

**Sample input:**
N/A

**Sample output:**
N/A

**Reflection:**

*Reflect on what you have learnt from this exercise.*
That there are still many ways to improve the performance and safety of the BigString class, especially by implementing move semantics and considering exception safety.


*Did you make any mistakes?*
No

*In what way has your knowledge improved?*
In that by researching for this question I now have a better understanding of advanced C++ concepts like move semantics, copy-and-swap idiom, and exception safety, which are crucial for writing efficient and robust C++ code.

**Questions:**

*Is there anything you would like to ask?*
No

### Final Reflection
For this project I got to understand the gang of 3 and how to stop memory allocation and research about more improvements to be made.

---

## Lab G
### Week 7 - Lab G

14 Nov 2025

#### Introduction
The aim of this lab is to become familiar with SIMD programming in C++. You will implement a simple vector mathematics library using SIMD instructions, and then use this library to perform some basic vector operations.


#### Q1. Benchmarks

**Question:**
Having created your `vector3d` and `matrix33d` classes in a previous lab, it is now time to optimise them by making use of the SIMD instructions on your CPU.

Before we start, we need to produce a set of reliable, reproducible and effective benchmarks.

You can use the project **Benchmarking** as the basis of your code

> Hint: Select a few of the matrix and vector operations, and combine them together to perform some simple matrix/vector arithmetic. Use the timing code from an earlier lab to provide an accurate measure of the duration.

**Solution:**
Here's the payload test:
```cpp
// BEGIN payload
// Create sample vectors
Vector3d v1(100.0, 200.0, 300.0);
Vector3d v2(400.0, 500.0, 600.0);

// Vector arithmetic
Vector3d v3 = v1 + v2;
Vector3d v4 = v3 - v1;
double dot = v1 * v2;
Vector3d cross = v1 ^ v2;
double len = v4.length();
v4.normalize();

// Create sample matrices
Matrix33d m1(
	Vector3d(100.0, 200.0, 300.0),
	Vector3d(400.0, 500.0, 600.0),
	Vector3d(700.0, 800.0, 900.0)
);
Matrix33d m2(
	Vector3d(900.0, 800.0, 700.0),
	Vector3d(600.0, 500.0, 400.0),
	Vector3d(300.0, 200.0, 100.0)
);

// Matrix arithmetic
Matrix33d m3 = m1 + m2;
Matrix33d m4 = m1 * m2;
Matrix33d m5 = m4.transpose();

// Matrix inversion (avoid singular matrix)
Matrix33d m6(
	Vector3d(2.0, -1.0, 0.0),
	Vector3d(-1.0, 2.0, -1.0),
	Vector3d(0.0, -1.0, 2.0)
);
Matrix33d inv = m6.inverse();

// Matrix-vector multiplication
Vector3d v5 = m6[0] + m6[1] + m6[2];
Vector3d v6 = v5 * inv;

// Dummy value to prevent optimization
dummyX += v6.length() + dot + cross.length() + m3[0].length() + m4[1].length() + m5[2].length() + inv[0].length();
// END payload
```
**Sample input:**
- For the Vectors:
```cpp
// Create sample vectors
Vector3d v1(100.0, 200.0, 300.0);
Vector3d v2(400.0, 500.0, 600.0);
```

- For the Matrices:
```cpp
// Create sample matrices
Matrix33d m1(
	Vector3d(100.0, 200.0, 300.0),
	Vector3d(400.0, 500.0, 600.0),
	Vector3d(700.0, 800.0, 900.0)
);
Matrix33d m2(
	Vector3d(900.0, 800.0, 700.0),
	Vector3d(600.0, 500.0, 400.0),
	Vector3d(300.0, 200.0, 100.0)
);
```

**Sample output:**

```
Overhead duration: 78

Median duration: 3266

Mean (80%) duration: 3272.23

Sample data [100]
........................
3196 3198 3198 3200 3200 3202 3202 3202 3202 3202 3202 3202 3202 3202 3202 3204 3204 3204 3204 3204 3204 3204 3204 3204 3204 3206 3206 3206 3206 3206 3206 3206 3206 3206 3206 3206 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3208 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210 3210
........................
18068 18094 18268 18724 18780 19174 19364 19716 20046 20430 20514 20526 20582 20606 20744 20796 20880 20956 21324 21450 21534 21764 21902 22196 22284 22310 22326 22328 22424 22550 22648 22684 22920 22956 23158 23458 23526 23526 23662 23980 24208 24208 24352 24394 24534 24906 25292 25352 25482 25598 25810 25838 25856 25912 26566 26800 26924 27186 27268 27772 28298 28738 28868 29310 29944 30106 30232 30420 30460 30518 30664 30810 31106 31478 32026 32286 32428 33660 33728 34434 35576 36024 36138 36642 36766 37296 37500 37896 38796 40386 41288 43596 43822 44794 55910 64186 91948 128598 129864 1346828
........................

x= 6.68477e+11 (dummy value - ignore)
```

**Reflection:**
*Reflect on what you have learnt from this exercise.*
I created the benchmarks for the vector and matrix operations using my previously implemented `vector3d` and `matrix33d` classes. I needed to ensure that the benchmarks were reliable and reproducible, so I can compare the performance of different implementations later on.

*Did you make any mistakes?* 

*In what way has your knowledge improved?*
In the sense that I practiced again how to create benchmarks, and I assured that the duration was not 1 as previous exercises.

**Questions:**
*Is there anything you would like to ask?*
No.


#### Q2. DirectXMath or GLM

**Question:**
Now we're going to look at DirectXMath (or GLM) and use it to replicate the benchmark you created in question 1.

Make a copy of your vector/matrix benchmark within the **DirectXMathsBenchmark** (or **GLMBenchmark**) project. Replace your vector and matrix classes with the equivalent DirectXMath (or GLM) classes.

Time your results. How did they compare to your original implementation?

**Solution:**

```cpp
// BEGIN payload
using namespace DirectX;

// Create sample vectors
XMVECTOR v1 = XMVectorSet(100.0f, 200.0f, 300.0f, 0.0f);
XMVECTOR v2 = XMVectorSet(400.0f, 500.0f, 600.0f, 0.0f);

// Vector arithmetic
XMVECTOR v3 = XMVectorAdd(v1, v2);
XMVECTOR v4 = XMVectorSubtract(v3, v1);
float dot = XMVectorGetX(XMVector3Dot(v1, v2));
XMVECTOR cross = XMVector3Cross(v1, v2);
float len = XMVectorGetX(XMVector3Length(v4));
v4 = XMVector3Normalize(v4);

// Create sample matrices
XMMATRIX m1(
	XMVectorSet(100.0f, 200.0f, 300.0f, 0.0f),
	XMVectorSet(400.0f, 500.0f, 600.0f, 0.0f),
	XMVectorSet(700.0f, 800.0f, 900.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
);
XMMATRIX m2(
	XMVectorSet(900.0f, 800.0f, 700.0f, 0.0f),
	XMVectorSet(600.0f, 500.0f, 400.0f, 0.0f),
	XMVectorSet(300.0f, 200.0f, 100.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
);

// Matrix arithmetic
XMMATRIX m3 = m1 + m2;
XMMATRIX m4 = XMMatrixMultiply(m1, m2);
XMMATRIX m5 = XMMatrixTranspose(m4);

// Matrix inversion (avoid singular matrix)
XMMATRIX m6(
	XMVectorSet(2.0f, -1.0f, 0.0f, 0.0f),
	XMVectorSet(-1.0f, 2.0f, -1.0f, 0.0f),
	XMVectorSet(0.0f, -1.0f, 2.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
);
XMMATRIX inv = XMMatrixInverse(nullptr, m6);

// Matrix-vector multiplication
XMVECTOR v5 = m6.r[0] + m6.r[1] + m6.r[2];
XMVECTOR v6 = XMVector3Transform(v5, inv);

// Dummy value to prevent optimization
dummyX += XMVectorGetX(XMVector3Length(v6)) + dot +
	XMVectorGetX(XMVector3Length(cross)) +
	XMVectorGetX(XMVector3Length(m3.r[0])) +
	XMVectorGetX(XMVector3Length(m4.r[1])) +
	XMVectorGetX(XMVector3Length(m5.r[2])) +
	XMVectorGetX(XMVector3Length(inv.r[0]));
// END payload
```

**Sample input:**
- For the Vector:
```cpp
// Create sample vectors
XMVECTOR v1 = XMVectorSet(100.0f, 200.0f, 300.0f, 0.0f);
XMVECTOR v2 = XMVectorSet(400.0f, 500.0f, 600.0f, 0.0f);
```
- For the Matrices:
```cpp
// Create sample matrices
XMMATRIX m1(
	XMVectorSet(100.0f, 200.0f, 300.0f, 0.0f),
	XMVectorSet(400.0f, 500.0f, 600.0f, 0.0f),
	XMVectorSet(700.0f, 800.0f, 900.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
);
XMMATRIX m2(
	XMVectorSet(900.0f, 800.0f, 700.0f, 0.0f),
	XMVectorSet(600.0f, 500.0f, 400.0f, 0.0f),
	XMVectorSet(300.0f, 200.0f, 100.0f, 0.0f),
	XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
);
```

**Sample output:**
```
Number of iterations: 250000

Overhead duration: 78

Median duration: 134

Mean (80%) duration: 133.758

Sample data [100]
........................
128 128 128 128 128 128 128 128 128 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130 130
........................
222 222 226 226 228 236 238 244 246 246 254 254 256 260 260 262 262 264 266 268 270 270 272 272 272 274 276 276 276 280 280 282 284 284 286 290 292 292 296 296 296 298 298 302 302 302 304 304 306 312 312 314 314 316 326 328 332 350 352 366 372 384 384 396 396 412 414 414 418 422 426 428 432 436 438 444 446 446 446 448 456 458 462 464 508 532 560 630 864 2108 3686 6398 11782 14926 18452 18694 20068 20260 28386 32018
........................

x= 6.68477e+11 (dummy value - ignore)
```
**Reflection:**

*Reflect on what you have learnt from this exercise.*
According to the results , the DirectXMath implementation is significantly faster than my original implementation. The median duration dropped from 3266 to 134, indicating a substantial performance improvement.

*Did you make any mistakes?*
Not really, but I had to be careful to ensure that I was using the correct DirectXMath functions for each operation.

*In what way has your knowledge improved?*
In that I have learned how to use the DirectXMath library to perform vector and matrix operations efficiently.

**Questions:**

*Is there anything you would like to ask?*
No.

#### Q3. SIMD (optional)

**Question:**
Now that you have your benchmarks, you can convert your vector and matrix classes to use the SIMD instructions.

Select one of the 3 methods discussed in the lecture (i.e. intrinsics, C++ classes or ISPC) and change the data members and functionality within the vector and matrix classes to use the selected approach.

Time your results. How did they compare to your original implementation?

**Solution:**
I ended up using intrinsics for this exercise.


```cpp
#include <xmmintrin.h> // SSE

// BEGIN payload

// Vectors as float arrays
alignas(16) float v1[4] = {100.0f, 200.0f, 300.0f, 0.0f};
alignas(16) float v2[4] = {400.0f, 500.0f, 600.0f, 0.0f};

// Load vectors into __m128
__m128 vec1 = _mm_load_ps(v1);
__m128 vec2 = _mm_load_ps(v2);

// Vector addition
__m128 v3 = _mm_add_ps(vec1, vec2);

// Vector subtraction
__m128 v4 = _mm_sub_ps(v3, vec1);

// Dot product (only first 3 components)
__m128 mul = _mm_mul_ps(vec1, vec2);
float dot = mul.m128_f32[0] + mul.m128_f32[1] + mul.m128_f32[2];

// Cross product (manual, as SSE has no direct cross)
float cross[3] = {
    v1[1]*v2[2] - v1[2]*v2[1],
    v1[2]*v2[0] - v1[0]*v2[2],
    v1[0]*v2[1] - v1[1]*v2[0]
};

// Length
float len = sqrtf(v4.m128_f32[0]*v4.m128_f32[0] +
                  v4.m128_f32[1]*v4.m128_f32[1] +
                  v4.m128_f32[2]*v4.m128_f32[2]);

// Normalize
__m128 norm_factor = _mm_set1_ps(len > 0.0f ? 1.0f/len : 0.0f);
__m128 v4_norm = _mm_mul_ps(v4, norm_factor);

// Matrices as float arrays (row-major, 3x3)
alignas(16) float m1[9] = {
    100.0f, 200.0f, 300.0f,
    400.0f, 500.0f, 600.0f,
    700.0f, 800.0f, 900.0f
};
alignas(16) float m2[9] = {
    900.0f, 800.0f, 700.0f,
    600.0f, 500.0f, 400.0f,
    300.0f, 200.0f, 100.0f
};
float m3[9], m4[9], m5[9], inv[9];

// Matrix addition
for (int i = 0; i < 9; ++i)
    m3[i] = m1[i] + m2[i];

// Matrix multiplication
for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
        m4[row*3 + col] = 0.0f;
        for (int k = 0; k < 3; ++k)
            m4[row*3 + col] += m1[row*3 + k] * m2[k*3 + col];
    }
}

// Matrix transpose
for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col)
        m5[col*3 + row] = m4[row*3 + col];

// Matrix inversion (same as previous, for brevity)
float det =
    m1[0] * (m1[4]*m1[8] - m1[5]*m1[7]) -
    m1[1] * (m1[3]*m1[8] - m1[5]*m1[6]) +
    m1[2] * (m1[3]*m1[7] - m1[4]*m1[6]);
if (fabsf(det) > 1e-8f) {
    inv[0] =  (m1[4]*m1[8] - m1[5]*m1[7]) / det;
    inv[1] = -(m1[1]*m1[8] - m1[2]*m1[7]) / det;
    inv[2] =  (m1[1]*m1[5] - m1[2]*m1[4]) / det;
    inv[3] = -(m1[3]*m1[8] - m1[5]*m1[6]) / det;
    inv[4] =  (m1[0]*m1[8] - m1[2]*m1[6]) / det;
    inv[5] = -(m1[0]*m1[5] - m1[2]*m1[3]) / det;
    inv[6] =  (m1[3]*m1[7] - m1[4]*m1[6]) / det;
    inv[7] = -(m1[0]*m1[7] - m1[1]*m1[6]) / det;
    inv[8] =  (m1[0]*m1[4] - m1[1]*m1[3]) / det;
}

// Matrix-vector multiplication
float v5[3] = {
    m1[0] + m1[3] + m1[6],
    m1[1] + m1[4] + m1[7],
    m1[2] + m1[5] + m1[8]
};
float v6[3] = {0.0f, 0.0f, 0.0f};
for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col)
        v6[row] += v5[col] * inv[row*3 + col];

// Dummy value to prevent optimization
dummyX += sqrtf(v6[0]*v6[0] + v6[1]*v6[1] + v6[2]*v6[2]) + dot +
          sqrtf(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]) +
          sqrtf(m3[0]*m3[0] + m3[1]*m3[1] + m3[2]*m3[2]) +
          sqrtf(m4[3]*m4[3] + m4[4]*m4[4] + m4[5]*m4[5]) +
          sqrtf(m5[6]*m5[6] + m5[7]*m5[7] + m5[8]*m5[8]) +
          sqrtf(inv[0]*inv[0] + inv[1]*inv[1] + inv[2]*inv[2]);
// END payload
```

**Sample input:**
- For the Vector:
```cpp
// Vectors as float arrays
alignas(16) float v1[4] = { 100.0f, 200.0f, 300.0f, 0.0f };
alignas(16) float v2[4] = { 400.0f, 500.0f, 600.0f, 0.0f };
```
- For the Matrices:
```cpp
// Matrices as float arrays (row-major, 3x3)
alignas(16) float m1[9] = {
	100.0f, 200.0f, 300.0f,
	400.0f, 500.0f, 600.0f,
	700.0f, 800.0f, 900.0f
};
alignas(16) float m2[9] = {
	900.0f, 800.0f, 700.0f,
	600.0f, 500.0f, 400.0f,
	300.0f, 200.0f, 100.0f
};

```

**Sample output:**
```
Number of iterations: 250000

Overhead duration: 76

Median duration: 450

Mean (80%) duration: 451.033

Sample data [100]
........................
446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446 446
........................
618 618 618 618 620 620 620 620 620 622 622 622 622 622 624 626 626 626 628 628 630 638 640 646 684 692 726 728 730 730 732 732 734 734 734 734 734 734 734 736 736 736 738 740 740 742 742 742 744 744 744 744 746 746 750 750 750 756 764 768 774 782 790 826 1228 1274 2270 2606 3004 3046 3396 3430 3520 3670 4038 5110 5294 6444 11964 13024 13100 13976 14068 14146 14288 14890 15960 16326 16688 17218 17330 17834 18452 19048 19882 20622 22206 30962 33770 81470
........................

x= 6.34107e+14 (dummy value - ignore)
```
**Reflection:**

*Reflect on what you have learnt from this exercise.*
I implemented the vector and matrix operations using SIMD intrinsics, as we know intrinsics expose the instructions seen on the SIMD set so we can use them in the algorithms, specifically SSE instructions.
This required careful handling of data alignment and understanding how to perform operations in parallel.

*Did you make any mistakes?*
Not really, but I had to ensure that the data was properly aligned for SIMD operations, which was a bit tricky at first.

*In what way has your knowledge improved?*
I have gained a deeper understanding of how SIMD works at a low level and how to leverage it for performance improvements in mathematical computations.

**Questions:**

*Is there anything you would like to ask?*
No

#### Q4. Profilers

**Question:**
Download the RayTracer.zip file from Canvas. We're now going to profile the Raytracer, using both a sampling and an instrumented profiler.

**Solution:**
I utilized the Visual Studio 2022 Performance Profiler to analyze the RayTracer application in **Release** mode in 32 bits.

**1. Sampling Profiler (CPU Usage)**
The sampling profiler takes snapshots of the call stack at regular intervals to estimate where time is spent.

* **Top Hotspot:** [`Sphere::intersect`, `Scene::render*`, `Plane::intersect`, `Erosion::intersect` and `[External Call] glut32.dll!0x000000001000a112]`]
* **Percentage of Self CPU [unit. %]:** [89.24%]

**2. Instrumented Profiler**
The instrumented profiler injects code to record exact call counts, resulting in higher overhead but precise frequency data.
* **Most Called Function:** [Insert Function Name]
* **Call Count:** [Insert Number, e.g., 15,400,200]
* **Observation:** I couldn't get to run the program properly with the instrumented profiler, as it kept crashing during execution.

**Sample input:**
Configuration: Visual Studio 2022, x86, Release Build.
Profiler Settings:
1. CPU Usage (Sampling) - Default interval.
2. Instrumentation - Default settings.

**Sample output:**
*Summary from Sampling Profiler:*
![Description of image](md_resources/screenshot_1.png)

*Summary from Instrumented Profiler:*

**Reflection:**

*Reflect on what you have learnt from this exercise.*
Well. Even thought I couldn't get the instrumented profiler to work properly, I learned how to use the sampling profiler effectively to identify performance bottlenecks in the RayTracer application. 
The sampling profiler provided insights into which functions were consuming the most CPU time, allowing me to focus on optimizing those areas.

*Did you make any mistakes?*
Well, I did try to fix the instrumented profiler issue , but I couldn't get it to work properly.

*In what way has your knowledge improved?*
In that the sampling profiler helps me out figuring out bottle necks and performance issues in code, which is a valuable skill for optimizing applications.

**Questions:**

*Is there anything you would like to ask?*
Yes, I would like to ask if there's any problems with the raytracing project that could cause the instrumented profiler to crash during execution, and if there are any known solutions or workarounds for this issue.

### Final Reflection
As mentioned in previous lab, this time around I tried to make the payload big enough for it to have meaningful data when benchmarking. This time around I learnd about the DirectXMath library and how to use it for vector and matrix operations, how
sometimes not "reinventing the wheel" is better, as libraries like DirectXMath are highly optimized and tested. Also, I learnt about SIMD programming using intrinsics, which gave me a deeper understanding of low-level optimizations and how to
leverage CPU capabilities for performance improvements. Lastly the use of profilers to identify performance bottlenecks in applications, which is a crucial skill for optimizing code effectively. It was quite interesting seeing all the different
approaches to optimization and performance analysis in C++ programming and how affecting the low level implementation can be for performance.

---

## Lab H
### Week 7 - Lab H

28 Nov 2025

#### Introduction
The aim of this lab is to extend the **BigString** class, developed during Lab F, to explore temporary anonymous objects.

#### Q1. BigString concatenators

**Question:**
Extend your **BigString** class to include the following new methods:

```c++
BigString& operator+= (const BigString& rhs);
BigString operator+ (const BigString& rhs) const;
```

Add further instrumentation to your code, by placing debug statements within each new method to stream out the name of the method e.g.

```c++
fout << "BigString( const & )"<< std::endl;
```

**Solution:**
- For the header file, BigString.h:
	```cpp
	#pragma once
	#include <iostream>
	#include <ostream>

	class BigString {
		char* _arrayOfChars;
		int _size;
	
	public:
		BigString();
		~BigString();

		BigString(const BigString& other);
		BigString& operator=(const BigString& other);
		BigString& operator+=(const BigString& rhs);
		BigString operator+(const BigString& rhs) const;

		char& operator[](int index);

		friend std::ostream& operator<<(std::ostream& os, const BigString& bs);
		friend std::istream& operator>>(std::istream& is, BigString& bs);
	};
	```

- For the body file, BigString.cpp:
	```cpp
	// OUTPUT STREAM OPERATOR
	std::ostream& operator<<(std::ostream& os, const BigString& bs) {
		os << bs._arrayOfChars;
		return os;
	}

	// INPUT STREAM OPERATOR
	std::istream& operator>>(std::istream& is, BigString& bs) {
		char buffer[1024];
		is >> buffer;
		int len = static_cast<int>(std::strlen(buffer));
		if (len >= bs._size) {
			delete[] bs._arrayOfChars;
			bs._size = len + 1;
			bs._arrayOfChars = new char[bs._size];
		}
		strcpy_s(bs._arrayOfChars, bs._size, buffer);
		return is;
	}
	```

**Reflection:**
*Reflect on what you have learnt from this exercise.*
I learnt how to implement operator overloading for string concatenation in C++. Also gained insights into memory management when dealing with dynamic character arrays.

*Did you make any mistakes?* 
No as far as I can tell, the implementation was straightforward and worked as intended. Alhough I had to be careful with memory allocation to avoid leaks this was fixed in the next exercise.

*In what way has your knowledge improved?*
In that I now have a better understanding of operator overloading and how to manage dynamic memory in C++ classes.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q2. Test harness

**Question:**
Extend your test hardness to include the new string concatenators, and test the two new methods.

How would you improve their efficiency?

Create an improved concatenator, taking ideas from the previous lecture on Vector3f.

Test your improved version.

> Hint: Look at not using operator overloads

**Solution:**
The test harness for the concatenators is as follows:
```cpp
// Create two large BigString objects
BigString bigA, bigB;

// Fill bigA with 5 'A's
for (int i = 0; i < 5; ++i) {
    if (i == 0) bigA += BigString();
    bigA[i] = 'A';
}
bigA[5] = '\0';

// Fill bigB with 5 'B's
for (int i = 0; i < 5; ++i) {
    if (i == 0) bigB += BigString();
    bigB[i] = 'B';
}
bigB[5] = '\0';

std::cout << "\nBigString bigA: " << bigA << std::endl;
std::cout << "BigString bigB: " << bigB << std::endl;

// Test operator+
BigString bigC = bigA + bigB;
std::cout << "bigC = bigA + bigB: " << bigC << std::endl;

// Test operator+=
bigA += bigB;
std::cout << "bigA += bigB: " << bigA << std::endl;

std::cout << "\nMain end" << std::endl;
return 0;
```

The results of Source.cpp test harness is extended as follows:
```cpp
Main start
BigString()
BigString()
BigString()
operator+=(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)
operator[](5)
BigString()
operator+=(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)
operator[](5)

BigString bigA: AAAAA
BigString bigB: BBBBB
operator+(const BigString&)
BigString() // HERE IS THE TEMPORARY OBJECT CREATED
bigC = bigA + bigB: AAAAABBBBB
operator+=(const BigString&)
bigA += bigB: AAAAABBBBB
```

**How would you improve their efficiency?**

It was detected that the current implementation of the concatenators is inefficient because it creates multiple temporary objects during concatenation, leading to unnecessary memory allocations and copies.
To improve efficiency, we can implement move semantics to avoid unnecessary copies and optimize memory usage:
- Pre-allocate a larger buffer if you expect many concatenations.
- Avoid using operator overloads for bulk concatenation; instead, implement a dedicated method like Concat(const BigString& rhs) that minimizes allocations.

The changes made to BigString.h:
```cpp
BigString Concat(const BigString& rhs) const;
```

The changes made to BigString.cpp:
```cpp
// CONCATENATE MEMBER FUNCTION
void BigString::Concat(const BigString& rhs) {
    std::cout << "Concat(const BigString&)" << std::endl;
    int newSize = std::strlen(_arrayOfChars) + std::strlen(rhs._arrayOfChars) + 1;
    if (newSize > _size) {
        char* newArray = new char[newSize];
        strcpy_s(newArray, newSize, _arrayOfChars);
        strcat_s(newArray, newSize, rhs._arrayOfChars);
        delete[] _arrayOfChars;
        _arrayOfChars = newArray;
        _size = newSize;
    } else {
        strcat_s(_arrayOfChars, _size, rhs._arrayOfChars);
    }
```

I changed the test to use the Concat function instead of the operator overloads:
```cpp
// Create two large BigString objects
BigString bigA, bigB;

// Fill bigA with 5 'A's
for (int i = 0; i < 5; ++i) {
    if (i == 0) bigA.Concat(BigString());
    bigA[i] = 'A';
}
bigA[5] = '\0';

// Fill bigB with 5 'B's
for (int i = 0; i < 5; ++i) {
    if (i == 0) bigB.Concat(BigString());
    bigB[i] = 'B';
}
bigB[5] = '\0';

std::cout << "\nBigString bigA: " << bigA << std::endl;
std::cout << "BigString bigB: " << bigB << std::endl;

// Test Concat (instead of operator+)
BigString bigC = bigA;
bigC.Concat(bigB);
std::cout << "bigC = bigA.Concat(bigB): " << bigC << std::endl;

// Test Concat (instead of operator+=)
bigA.Concat(bigB);
std::cout << "bigA.Concat(bigB): " << bigA << std::endl;

std::cout << "\nMain end" << std::endl;
return 0;
```

And the output is as follows:
```cpp
Main start
BigString()
BigString()
BigString()
Concat(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)
operator[](5)
BigString()
Concat(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)
operator[](5)

BigString bigA: AAAAA
BigString bigB: BBBBB
BigString(const BigString&)
Concat(const BigString&)
bigC = bigA.Concat(bigB): AAAAABBBBB
Concat(const BigString&)
bigA.Concat(bigB): AAAAABBBBB
```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt how to optimize string concatenation by implementing a dedicated method that minimizes memory allocations and copies, improving performance over operator overloads.

*Did you make any mistakes?*
No significant mistakes were made, but I had to ensure proper memory management to avoid leaks during concatenation.

*In what way has your knowledge improved?*
In that memory allocation and performance considerations are crucial when designing classes that handle dynamic data. That's why I had to implement the Concat method to improve efficiency as explained above.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q3. BigString move constructor and operator

**Question:**

**Solution:**
- On the BigString.h:
	```cpp
	#pragma once
	#include <iostream>
	#include <ostream>

	class BigString {
		char* _arrayOfChars;
		int _size;
	
	public:
		BigString();
		~BigString();

		BigString(const BigString& other);
		BigString& operator=(const BigString& other);
		BigString& operator+=(const BigString& rhs);
		BigString operator+(const BigString& rhs) const;

		BigString(BigString&& other) noexcept;
		BigString& operator=(BigString&& other) noexcept;

		char& operator[](int index);

		friend std::ostream& operator<<(std::ostream& os, const BigString& bs);
		friend std::istream& operator>>(std::istream& is, BigString& bs);

		void Concat(const BigString& rhs);
	};
	```

- On the BigString.cpp:
	```cpp
	#include <fstream>

	static std::ofstream fout("BigString_debug.log", std::ios::app); 
 
	...

	// MOVE CONSTRUCTOR
	BigString::BigString(BigString&& other) noexcept
		: _arrayOfChars(other._arrayOfChars), _size(other._size)
	{
		fout << "BigString(const &&)" << std::endl;
		other._arrayOfChars = nullptr;
		other._size = 0;
	}

	// MOVE ASSIGNMENT OPERATOR
	BigString& BigString::operator=(BigString&& other) noexcept
	{
		fout << "operator=(const &&)" << std::endl;
		if (this != &other) {
			delete[] _arrayOfChars;
			_arrayOfChars = other._arrayOfChars;
			_size = other._size;
			other._arrayOfChars = nullptr;
			other._size = 0;
		}
		return *this;
	}
	```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt how to implement move semantics in C++ to optimize resource management and performance when dealing with temporary objects. This is particularly useful for classes that manage dynamic memory, like BigString.

*Did you make any mistakes?*
No as far as I can tell.

*In what way has your knowledge improved?*
In that the move constructor and move assignment operator allow for efficient transfer of resources from temporary objects, reducing unnecessary copies and improving performance.

**Questions:**
*Is there anything you would like to ask?*
No.

#### Q4. Test harness

**Question:**
Extend your test hardness to include the two new methods.

How much performance improvement do you get using the move rather than classical functions?

**Solution:**
The test ended up as follows:
```cpp
// --- Q3: Test BigString move semantics ---

// Move constructor test
BigString moveSource;
moveSource.Concat(BigString());
moveSource[0] = 'M';
moveSource[1] = 'o';
moveSource[2] = 'v';
moveSource[3] = 'e';
moveSource[4] = '\0';

std::cout << "\nBefore move, moveSource: " << moveSource << std::endl;
BigString movedTo(std::move(moveSource)); // Move constructor
std::cout << "After move, movedTo: " << movedTo << std::endl;
std::cout << "After move, moveSource (should be empty or in valid state): " << moveSource << std::endl;

// Move assignment test
BigString moveAssignSource;
moveAssignSource.Concat(BigString());
moveAssignSource[0] = 'A';
moveAssignSource[1] = 's';
moveAssignSource[2] = 's';
moveAssignSource[3] = 'i';
moveAssignSource[4] = 'g';
moveAssignSource[5] = 'n';
moveAssignSource[6] = '\0';

BigString moveAssigned;
moveAssigned = std::move(moveAssignSource); // Move assignment
std::cout << "After move assignment, moveAssigned: " << moveAssigned << std::endl;
std::cout << "After move assignment, moveAssignSource (should be empty or in valid state): " << moveAssignSource << std::endl;
```

**Sample output:**

```
Main start
BigString()
BigString()
Concat(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)

Before move, moveSource: Move
After move, movedTo: Move
After move, moveSource (should be empty or in valid state):
BigString()
BigString()
Concat(const BigString&)
~BigString()
operator[](0)
operator[](1)
operator[](2)
operator[](3)
operator[](4)
operator[](5)
operator[](6)
BigString()
After move assignment, moveAssigned: Assign
After move assignment, moveAssignSource (should be empty or in valid state):

Main end
~BigString()
~BigString()
~BigString()
~BigString()
```

In the *BigString_debug.log* file it printed out the following:
```
BigString(BigString&&) called
BigString::operator=(BigString&&) called

```

**Reflection:**

*Reflect on what you have learnt from this exercise.*
I learnt how to effectively test move semantics in C++ by creating temporary objects and using the move constructor and move assignment operator. This helped me understand how resources are transferred without unnecessary copying.

*Did you make any mistakes?*
Yes, The "<<" operator implementation did not handle the case when the BigString object was in a moved-from state (i.e., when its internal character array pointer was null). This oversight led to undefined behavior when attempting to print a moved-from BigString.
The new implementation was:
```cpp
// OUTPUT STREAM OPERATOR
std::ostream& operator<<(std::ostream& os, const BigString& bs) {
    // Check if the buffer is valid before printing to avoid Access Violation
    if (bs._arrayOfChars != nullptr) {
        os << bs._arrayOfChars;
    }
    return os;
}
```

*In what way has your knowledge improved?*
In that I now have a deeper understanding of move semantics and how they can significantly improve performance by avoiding unnecessary copies, especially in classes that manage dynamic memory.

**Questions:**
*Is there anything you would like to ask?*
No.

### Final Reflection
This lab work helped me understand the importance of operator overloading, move semantics, and efficient memory management in C++. Implementing and testing these concepts in the BigString class provided practical experience that reinforced my theoretical knowledge.
Overall, the lab was a valuable learning experience that enhanced my C++ programming skills, particularly in designing classes that handle dynamic data efficiently.