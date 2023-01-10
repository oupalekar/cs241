## Chapter 1

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".
    ```
        #include <unistd.h>

        int main() {
            write(1, "Hi! My name is Ojas", 19);
            return 0;
        }
    ```
### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```
   ```
        #include <unistd.h>

        int write_triangle(int n) {
            int x = 1;
            for(int i = 0; i < n; i++) {
                for(int j = 0; j < x j++) {
                    write(1, "*", 1);
                }
                write(1, "\n", 1);
                j++;
            }
        }

        int main() {
            write_triangle(3);
            return 0;
        }
   ```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).

```
    #include <fcntl.h>
    #include <unistd.h>
    int main() {
        mode_t mode = S_IRUSR | S_IWUSR;
        int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
        write(fildes, "Hello, World!", 13);
        close(fildes);
        return 0;
    }
```


### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
```
    #include <fcntl.h>
    #include <unistd.h>
    #include <stdio.h>
    int main() {
        mode_t mode = S_IRUSR | S_IWUSR;
        close(1);
        int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
        printf("Hello, World %d!\n ", 16);
        close(fildes);
        return 0;
    }
```

5. What are some differences between `write()` and `printf()`?
    
    write() is a system call so it talks directly to the kernel of your system. However, write() is very basic, as it can only print out a sequence of bytes. printf() uses write() and is built around it but allows you to print more complex data types.



## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?

There are at least 8 bits in a byte.

2. How many bytes are there in a `char`?

A char is one byte

3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`

    sizeof an int: 4 bytes
    sizeof an double: 8 bytes
    sizeof an float: 4 bytes
    sizeof an long: 8 bytes
    sizeof an long long: 8 bytes

Code used:
```
#include <stdio.h>
int main() {
	printf("sizeof an int: %lu\n", sizeof(int));	
	printf("sizeof an double: %lu\n", sizeof(double));
	printf("sizeof an float: %lu\n", sizeof(float));
	printf("sizeof an long: %lu\n", sizeof(long));
	printf("sizeof an long long: %lu\n", sizeof(long long));
	return 0;
}
```

### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?

The address of the data would be `0x7fbd9d50`

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?

   `data[3]` is equivalent to `*(data + 3)` 

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
The code segfaults because the `"hello"` that is assigned to the `char` pointer is in read-only and constant memory.

7. What does `sizeof("Hello\0World")` return?

12

8. What does `strlen("Hello\0World")` return?

5

9. Give an example of X such that `sizeof(X)` is 3.

X = "12", X is a string of two characters, with the 3rd character being the `\0` at the end.

10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.

Y is a pointer.


## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?

You can either check the value of argc, or loop through the contents of argv and have a counter that increments every times the loop runs.

2. What does `argv[0]` represent?

`argv[0]` represents the name of program.

### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?

The pointers to environment variables are stored in a pointer called `environ`. This generally is above the stack in memory

### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?

`sizeof(ptr)` will be 8 bytes since this architecture has pointers of 8 bytes. However, `sizeof(array)` will be 6.

### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?

The stack.

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?

In the heap using `malloc`

2. What are the differences between heap and stack memory?

Heap memory is memory that can be accessed throughout the process. Stack memory is restricted to the scope of the subprocess or function.

3. Are there other kinds of memory in a process?

Yes, your code is stored in the text portion, and there are global variables and static variables that are also stored in places other than the stack and heap.

4. Fill in the blank: "In a good C program, for every malloc, there is a ___".

Free 

### Heap allocation gotchas
5. What is one reason `malloc` can fail?

A reason why `malloc` may fail is allocating 0 bytes when calling `malloc`. Another reason is if there is no more memory that can be allocated by `malloc`.

6. What are some differences between `time()` and `ctime()`?

`time()` will return the number of seconds since 1970 (considered the start of this Epoch). `ctime()` allows us to convert those seconds by passing in the reference to the output of `time()` into readable time (I.E. 2:31PM)

7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```

We can't free anything from `ptr` on the second `free` since `ptr` has no memory allocated to it.

8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```

Since `free(ptr)` was called, the `ptr` is not pointing to anything and has no memory allocated to it. So calling it in `printf` will result in an error.

9. How can one avoid the previous two mistakes?

Set `ptr = NULL` after calling `free()`

### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).

```
    struct Person {
        char *name;
        int age;
        struct Person *friends;
    };

    typedef struct Person person_t;
```

11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.

```
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
struct Person {
        char *name;
        int age;
        struct Person** friends;
    };
    typedef struct Person person_t;

    int main() {
        person_t* person1 = (person_t*) malloc(sizeof(person_t));
        person_t* person2 = (person_t*) malloc(sizeof(person_t));
        person1->friends =(person_t*) malloc(sizeof(person_t) * 10); // 10 friends
        person2->friends =(person_t*) malloc(sizeof(person_t) * 10);

        person1->name = "Agent Smith";
        person1->age = 128;
        *(person1->friends) = *person2;

        person1->name = "Sonny Moore";
        person1->age = 256;
        *(person1->friends) = *person1;
    }
```
### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).

```
person_t* person_create(char* aname, int aage, person_t ** afriends) {
    person_t *result = malloc(sizeof(person_t));
    result-> name = strdup(aname);
    result-> age = aage;
    result->friends =(person_t*) malloc(sizeof(person_t) * 10);
    return result;
}

void person_destroy(person_t * p) {
    memset(p, 0, sizeof(person_t));
    free(p);
}
```
12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).

13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.

## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.
### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?

To get characters from `stdin` we can use `gets()` and then we can write them out to `stdout` using `puts`

2. Name one issue with `gets()`.

`gets()` doesn't shield us from overriding memory. Thus, if your input is larger than the buffer that is the parameter to `gets()` it will overide memory.

### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".

```
int main() {
    char * data = "Hello 5 World";
    char bufone[6];
    int five = -5;
    char buftwo[6];

    sscanf(data, "%s %d %s", bufone, &five, buftwo);
    printf("%s", bufone); 
}
```
### `getline` is useful
4. What does one need to define before including `getline()`?

Must define `#define _GNU_SOURCE`


5. Write a C program to print out the content of a file line-by-line using `getline()`.

```
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>


int main() {
        FILE * file = NULL;
        char * line = NULL;
        size_t lineno = 0;
        //ssize_t linechar = NULL;      

        file = fopen("random.txt", "r");
        while ( getline(&line, &lineno, file)!= -1) {
                printf("%s\n", line);
        }
        fclose(file)
        return 0;

}
```

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?

`-g`

2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.

You need to delete the cached files from previous builds. That's why most make files have a `clean` executable to remove all the `.o` files that have been created by previous builds. 

3. Are tabs or spaces used to indent the commands after the rule in a Makefile?

Tabs are used to indent the commands after the rule in a Makefile

4. What does `git commit` do? What's a `sha` in the context of git?

`git commit` will create a new commit in the version control for that branch. `sha` provides you with a unique identifier for that commit as a checksum

5. What does `git log` show you?

`git log` provides a log of all the commits, their branches, authors and date of commit

6. What does `git status` tell you and how would the contents of `.gitignore` change its output?

`git status` shows you what files are untracked and have not been commited, and shows whether your branch is up to date with the top branch (master)

Adding files `.gitignore` will tell git to ignore those files when committing. They will not show up when you call `git status`

7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?

`git push` will push all commits from a local repo to a remote repo (ie github). It is not sufficient to simply commit because you must call `git add` to tell git what files must be commited.

8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?

A non-fast-forward error `git push` means that the remote repository has more commits than the local repo, which means your code is outdated. The most common way of dealing is to stash your code using `git stash`, calling `git fetch` to get the code from remote repo, then type `git stash pop` to return your code, and fix the merges.