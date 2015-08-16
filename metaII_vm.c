/*

META II VM in C

	"To move Meta II to a different language these and only these
	 are the objects and operations that must be implemented in the
	target language."

		-- James M. Neighbors

		

C language VM code
------------------
Copyright 2011, Daniel Chapiesky - C Version of META II VM  (dchapiesky at yahoo)

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice, URL, and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


Original META II VM code
------------------------

mc_semantics.html from mc_tutorial.zip (http://www.bayfronttechnologies.com/mc_tutorial.zip)

Original code in Javascript

Copyright 2006, 2008 Bayfront Technologies, Inc. (James M. Neighbors) http://www.BayfrontTechnologies.com/mc_workshop.html

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice, URL, and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


--------------------------------------------------------------------------------------------------------

// 2011-07-16 (DCC) translate original JMN javascript VM into C with additional supporting code

// 2008-08-18 (JMN) create out function for margin handling
// 2007-06-10 (JMN) changed compare code to out, allowed EOF whitespace
// 2006-10-25 (JMN) eliminated per character output
// 2006-09-27 (JMN) addition of code for tokens
// 2006-07-31 (JMN) addition of code and output comparison
// 2006-07-29 (JMN) addition of runext minor extension order codes
// 2006-07-13 (JMN) minor changes for IE and Mozilla compatability
// 2006-07-12 (JMN) new javascript from pre 1990 JMN pascal code

--------------------------------------------------------------------------------------------------------

The Source Code
===============

	Source code has 4 byte tabs

	Source code formatted with: astyle --indent=tab --brackets=linux --indent-switches --indent-preprocessor --break-blocks *.c

	Source code documented using doxygen (www.doxygen.org)

--------------------------------------------------------------------------------------------------------

Modifications forced by the translation process:
================================================

	1)	stack, stackframe, stackframesize, & stack[current_stackframe_ptr * stackframesize + N] became:

			current_stackframe_ptr

 		where current_stackframe_ptr points to a structure with the stackframesize number of elements.

	2)	variable names were replaced with more readable ones (since I actually have to understand this code to translate it into C)

	3)	dynamic strings were used to support closer functionality of the original Javascript code (optimizations clearly can be made)


--------------------------------------------------------------------------------------------------------

Notes on pointers to functions in structures...
or
Why some things are the way they are in my code...
==================================================

	Ya'll know about classes and C++... yes?

	The C++ code:

		class thatThing {
			void doSomething();
		};

	allows expressions such as...

		thatThing *myThing;	

		...

		myThing->doSomething();

	I like this syntax for working with structures. So, irrelevent of the overhead, I do the following quite a bit in C:

		typedef struct thatThing_struct {
			void (*doSomething)(thatThing_struct *me);
		} thatThing;

		thatThing* thatThing_new() {
			...
			myNewThing->doSomething = thatThing_doSomething;
			...
			return myNewThing;
		}

		void thatThing_doSomething(thatThing_struct *me) {
			...
		}

		...

		thatThing *myThing;

		myThing = thatThing_new();

		myThing->doSomething(myThing);

	Don't try to say it's OO or anything like that.  It isn't even like what cfont used to chuck out.  Every instance of a structure has it's own
	private lookup table.  You get massive memory overhead, but every instance can have it's behaviours uniquely modified without all that cheesy
	mucking around with whatever C++ is using now.

	In the end, I just like to read:

		myThing->doSomething(myThing);

*/


// ========================================================================================================================================================================
// ========================================================================================================================================================================
// ========================================================================================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>

// This file contains the Input & Code sources used in the meta compiler workshop 
// http://www.bayfronttechnologies.com/mc_workshop.html
// http://www.bayfronttechnologies.com/mc_tutorial.zip
// 
#include "mc_tutorial_sources.c"


// ========================================================================================================================================================================
// ========================================================================================================================================================================
// ========================================================================================================================================================================



// 
// metaII_memory_allocator
//
// A wrapper around malloc/realloc/free to do simple memory leak checking... 
//
// This is not an exhaustive test of all memory leaks, just unbalanced malloc/free 
// 
// One thing it does do is guarantee a region returned is all 0's


/// a simple memory allocator handle
///
/// Allows you to redefine the allocator system for use with pools, etc...
///
typedef struct metaII_memory_allocator_struct {

	/// pointer to function to allocate memory with
	///
	/// @param[in] 		allocator		the allocator to request memory from
	/// @param[in] 		theAmount		the size of the area to allocate
	///
	/// \retval	aValue	a pointer to the area allocated
	/// \retval NULL	there was a failure
	///
	void * (*allocate_memory)(struct metaII_memory_allocator_struct * allocator, size_t theAmount);

	/// pointer to function to free the memory with
	///
	/// @param[in] 		allocator	the allocator to release memory to
	/// @param[in] 		theMemory	pointer to the memory area to free
	///
	void (*free_memory)(struct metaII_memory_allocator_struct * allocator, char *theMemory);


	unsigned long long		total_allocs;					///< statistics - total allocs in time
	unsigned long long		total_frees;					///< total frees in time


	/// destroy this allocator
	///
	/// @param[in] 		allocator	the allocator to destroy
	///
	void (*destroy)(struct metaII_memory_allocator_struct * allocator);


	/// print the stats to the log
	/// we use a void * cause log has not yet been defined
	///
	/// @param[in] 		allocator	the allocator to access
	///
	void (*log_stats)(struct metaII_memory_allocator_struct *allocator);

	/// is the allocator balanced
	/// only applicable at end of execution
	/// does allocs == frees
	/// 
	/// @param[in] 		allocator	the allocator to check
	///
	unsigned long (*is_balanced)(struct metaII_memory_allocator_struct *allocator);



} metaII_memory_allocator;


#define metaII_memory_allocator_UPDATE_FREE(theAllocator, amount) \
	(theAllocator)->total_frees++;


#define metaII_memory_allocator_UPDATE_FREE_with_check(theAllocator, amount) \
	if ((theAllocator)->total_allocs) {\
		(theAllocator)->total_frees++;\
	} else {\
		fprintf(stderr,"%s: current allocs is 0 and someone wants to decrement it... amount is [%lu]",  my_function_name, amount);\
	}\



#define metaII_memory_allocator_UPDATE_ALLOC(theAllocator, amount) \
	(theAllocator)->total_allocs++;



metaII_memory_allocator *						metaII_memory_allocator_new();
void *											metaII_memory_allocator_allocate_memory(struct metaII_memory_allocator_struct *allocator, size_t theAmount);
void											metaII_memory_allocator_free_memory(struct metaII_memory_allocator_struct *allocator, char *theMemory);
void											metaII_memory_allocator_destroy(struct metaII_memory_allocator_struct *allocator);
void											metaII_memory_allocator_log_stats(struct metaII_memory_allocator_struct *allocator);
unsigned long									metaII_memory_allocator_is_balanced(struct metaII_memory_allocator_struct *allocator);



/// allocate memory using the OS/libc supplied malloc
/// \relates metaII_memory_allocator_struct
///
///
/// @param[in] 		allocator		the allocator to request memory from
/// @param[in] 		theAmount		the size of the area to allocate
///
/// \retval	aValue	a pointer to the area allocated
/// \retval NULL	there was a failure
///
void * metaII_memory_allocator_allocate_memory(struct metaII_memory_allocator_struct *allocator, size_t theAmount)
{
	void * theBuffer;

	if (!allocator) {
		return 0;
	}

	if (theAmount) {
		metaII_memory_allocator_UPDATE_ALLOC(allocator, theAmount);

		theBuffer = malloc(theAmount);
		memset(theBuffer, 0, theAmount);

		return theBuffer;
	}

	return 0;
}


/// free memory using the OS/libc supplied free
/// \relates metaII_memory_allocator_struct
///
/// @param[in] 		allocator	the allocator to release memory to
/// @param[in] 		theMemory	pointer to the memory area to free
///
void metaII_memory_allocator_free_memory(struct metaII_memory_allocator_struct *allocator, char *theMemory)
{

	if (!allocator) {
		return;
	}

	// in this allocator... we do not know the size of theMemory
	// so statistics for total bytes allocated will be off
	// the number of allocations however will be accurate
	//
	if (theMemory) {
		metaII_memory_allocator_UPDATE_FREE(allocator, 0);
		free(theMemory);
	} else {
		fprintf(stderr, "metaII_memory_allocator_free_memory: Passed NULL Pointer to free");
	}
}


/// create a memory allocator handle
/// for the OS Malloc/Free methods...
/// \relates metaII_memory_allocator_struct
///
metaII_memory_allocator *metaII_memory_allocator_new()
{
	metaII_memory_allocator *myNewAllocator;

	// get some memory from the OS
	//
	myNewAllocator = (metaII_memory_allocator *) malloc(sizeof(metaII_memory_allocator));

	if (!myNewAllocator) {
		return 0;
	}

	memset((char *) myNewAllocator, 0, sizeof(metaII_memory_allocator));


	// setup the function calls for this allocator
	//
	myNewAllocator->allocate_memory  =				metaII_memory_allocator_allocate_memory;
	myNewAllocator->free_memory      =				metaII_memory_allocator_free_memory;
	myNewAllocator->destroy			 =				metaII_memory_allocator_destroy;

	myNewAllocator->log_stats			 =				metaII_memory_allocator_log_stats;
	myNewAllocator->is_balanced			 =				metaII_memory_allocator_is_balanced;

	return myNewAllocator;
}

/// destroy this allocator
/// since OSMalloc does not track allocations anything that has not been freed is screwed...
/// \relates metaII_memory_allocator_struct
///
/// @param[in] 		allocator	the allocator to destroy
///
void metaII_memory_allocator_destroy(struct metaII_memory_allocator_struct *allocator)
{
	if (!allocator) {
		return;
	}

	// since it does not track memory
	// this allocator cannot free it's list of allocations... again cause it doesn't have one
	//
	memset(allocator, 0, sizeof(metaII_memory_allocator));
	free(allocator);
}



/// check to see if the memory allocator has an equal number of allocation calls and free calls
/// \relates metaII_memory_allocator_struct
///
/// @param[in] 		allocator	the allocator to check
///
unsigned long metaII_memory_allocator_is_balanced(struct metaII_memory_allocator_struct *allocator)
{
	return ((allocator->total_frees == allocator->total_allocs) ? 1 : 0);
}





/// place memory allocator statistics in the log
/// \relates metaII_memory_allocator_struct
///
/// @param[in] 		allocator	the allocator to access
///
void metaII_memory_allocator_log_stats(struct metaII_memory_allocator_struct *allocator)
{
	fprintf(stderr,"Total Allocs                   : %llu\n", allocator->total_allocs);
	fprintf(stderr,"Total Frees                    : %llu\n", allocator->total_frees);
	fprintf(stderr,"Allocator Balanced             : %s\n", ((allocator->total_frees == allocator->total_allocs) ? "Yes" : "No"));
}



// ========================================================================================================================================================================
// ========================================================================================================================================================================
// ========================================================================================================================================================================


// metaII_dynamic_string
//
// very simple string manager allowing appendment while maintaining a standard C char * to the string
//
// This is not an exhaustive string class, just bare bones for this vm...
//
typedef struct metaII_dynamic_string_struct {                         
	char *ptr;                             			///< pointer to the region 
	metaII_memory_allocator *theAllocator;  		///< the allocator used to allocate this buffer

	unsigned long string_length;                              ///< length of the string in the buffer
	unsigned long buf_size;                            		///< size of the allocated buffer

	struct metaII_dynamic_string_struct *	(*duplicate)(const struct metaII_dynamic_string_struct *theStr);
	void 									(*append)(struct metaII_dynamic_string_struct *theStr, const char *ptr, unsigned long string_length);
	void 									(*append_dyn)(struct metaII_dynamic_string_struct *theStr,  struct metaII_dynamic_string_struct *ptr);
	void 									(*clear)(struct metaII_dynamic_string_struct *theStr);
	void 									(*destroy)(struct metaII_dynamic_string_struct *theStr);
	void									(*log)(struct metaII_dynamic_string_struct *theStr, const char *header);
} metaII_dynamic_string;


#define metaII_dynamic_stringUNIT     128                // allocation unit size of an extensible string


metaII_dynamic_string *					metaII_dynamic_string_new(metaII_memory_allocator *theAllocator);
struct metaII_dynamic_string_struct *	metaII_dynamic_string_duplicate(const struct metaII_dynamic_string_struct *theStr);
void 									metaII_dynamic_string_destroy(struct metaII_dynamic_string_struct *theStr);
void 									metaII_dynamic_string_append(struct metaII_dynamic_string_struct *theStr, const char *ptr, unsigned long string_length);
void 									metaII_dynamic_string_append_dyn(struct metaII_dynamic_string_struct *theStr, struct metaII_dynamic_string_struct *ptr);
void 									metaII_dynamic_string_clear(struct metaII_dynamic_string_struct *theStr);
void 									metaII_dynamic_string_log(struct metaII_dynamic_string_struct *theStr, const char *header);


/* Create an extensible string object. */
metaII_dynamic_string *metaII_dynamic_string_new(metaII_memory_allocator *theAllocator)
{
	metaII_dynamic_string *theStr;

	theStr = (metaII_dynamic_string *) theAllocator->allocate_memory(theAllocator, sizeof(metaII_dynamic_string));
	theStr->ptr = (char *) theAllocator->allocate_memory(theAllocator, metaII_dynamic_stringUNIT);
	theStr->string_length = 0;
	theStr->buf_size = metaII_dynamic_stringUNIT;
	theStr->ptr[0] = '\0';

	theStr->duplicate = metaII_dynamic_string_duplicate;
	theStr->append = metaII_dynamic_string_append;
	theStr->append_dyn = metaII_dynamic_string_append_dyn;
	theStr->clear = metaII_dynamic_string_clear;
	theStr->destroy = metaII_dynamic_string_destroy;
	theStr->log = metaII_dynamic_string_log;

	theStr->theAllocator = theAllocator;

	return theStr;
}



/* Copy an extensible string object. */
metaII_dynamic_string *metaII_dynamic_string_duplicate(const struct metaII_dynamic_string_struct *theStr)
{
	metaII_dynamic_string *newStr;
	unsigned long buf_size;

	newStr = (metaII_dynamic_string *) theStr->theAllocator->allocate_memory(theStr->theAllocator, sizeof(metaII_dynamic_string));
	buf_size = theStr->buf_size;
	newStr->ptr = (char *) theStr->theAllocator->allocate_memory(theStr->theAllocator, buf_size);
	newStr->string_length = theStr->string_length;
	newStr->buf_size = buf_size;
	memcpy(newStr->ptr, theStr->ptr, theStr->string_length + 1);

	newStr->duplicate = metaII_dynamic_string_duplicate;
	newStr->append = metaII_dynamic_string_append;
	newStr->clear = metaII_dynamic_string_clear;
	newStr->destroy = metaII_dynamic_string_destroy;
	newStr->log = metaII_dynamic_string_log;

	newStr->theAllocator = theStr->theAllocator;

	return newStr;
}


void metaII_dynamic_string_destroy(struct metaII_dynamic_string_struct *theStr)
{
	metaII_memory_allocator *theAllocator;

	if (theStr) {
		theAllocator = theStr->theAllocator;

		if (theStr->ptr) {
			theAllocator->free_memory(theAllocator, (char *) theStr->ptr);
		}

		theAllocator->free_memory(theAllocator, (char *) theStr);
	}
}



void metaII_dynamic_string_append(struct metaII_dynamic_string_struct *theStr, const char *ptr, unsigned long string_length)
{
	unsigned long nsize;
	unsigned long xsize;
	char *newbuf;

	nsize = theStr->string_length + string_length + 1;

	xsize = theStr->buf_size;
	if(xsize < nsize) {
		while (xsize < nsize) {
			xsize *= 2;
	
			if (xsize < nsize) {
				xsize = nsize;
			}
		}
	}
	
	newbuf = theStr->theAllocator->allocate_memory(theStr->theAllocator, xsize);
	memcpy(newbuf, theStr->ptr, theStr->buf_size);
	
	theStr->theAllocator->free_memory(theStr->theAllocator, theStr->ptr);
	theStr->ptr = newbuf;
	theStr->buf_size = xsize;


	memcpy(theStr->ptr + theStr->string_length, ptr, string_length);
	theStr->string_length += string_length;
	theStr->ptr[theStr->string_length] = '\0';
}


void metaII_dynamic_string_append_dyn(struct metaII_dynamic_string_struct *theStr, struct metaII_dynamic_string_struct *ptr)
{
	unsigned long nsize;
	unsigned long xsize;
	char *newbuf;

	nsize = theStr->string_length + ptr->string_length + 1;

	xsize = theStr->buf_size;
	if(xsize < nsize) {
		while (xsize < nsize) {
			xsize *= 2;
	
			if (xsize < nsize) {
				xsize = nsize;
			}
		}
	}
	
	newbuf = theStr->theAllocator->allocate_memory(theStr->theAllocator, xsize);
	memcpy(newbuf, theStr->ptr, theStr->buf_size);
	
	theStr->theAllocator->free_memory(theStr->theAllocator, theStr->ptr);
	theStr->ptr = newbuf;
	theStr->buf_size = xsize;


	memcpy(theStr->ptr + theStr->string_length, ptr->ptr, ptr->string_length);
	theStr->string_length += ptr->string_length;
	theStr->ptr[theStr->string_length] = '\0';
}

void metaII_dynamic_string_clear(struct metaII_dynamic_string_struct *theStr)
{
	theStr->string_length = 0;
	theStr->ptr[0] = '\0';
}



void metaII_dynamic_string_log(struct metaII_dynamic_string_struct *theStr, const char *header)
{
	fprintf(stderr, "\n%s: (%lu,%lu)=[%s]\n\n", header, theStr->string_length, theStr->buf_size, theStr->ptr);
}




// ========================================================================================================================================================================
// ========================================================================================================================================================================
// ========================================================================================================================================================================

/// opcode (or order code) lookup table entry
///
/// Allows conversion of string opcode name to opcode value or jump address
///
typedef struct opcode_description_struct {
	unsigned char	opcode_value;				///< numeric value for opcode
	const char *	opcode_string;				///< name of opcode
	unsigned long	opcode_string_length;		///< length of the name of the opcode
	const char *	opcode_description;			///< description of opcode
	void *			goto_pointer; 				///< for gcc jump threading (not yet used...)
} opcode_description;


opcode_description *	opcode_bsearch(const opcode_description *base, unsigned long array_size, const char *key, unsigned long key_length);
int						opcode_bsearch_cmp(const opcode_description *a, const char *key, unsigned long key_length);


int opcode_bsearch_cmp(const opcode_description *a, const char *key, unsigned long key_length)
{
	unsigned char *ao;
	unsigned long size;
	unsigned long i;

	ao = (unsigned char *)((const opcode_description *)a)->opcode_string;

	// empty entry is obviously less than whatever is in key...
	//
	if (!ao) {
		return -1;
	}

 	size = ((((const opcode_description *)a)->opcode_string_length < key_length) ? ((const opcode_description *)a)->opcode_string_length : key_length);

	for (i = 0; i < size; i++) {
		if (ao[i] < key[i]) {
			return 1;
		}

		if (ao[i] > key[i]) {
			return -1;
		}
	}

	return (int) (key_length - ((const opcode_description *)a)->opcode_string_length);
}



// simple binary search based on the code from dietlibc 0.32 (Felix von Leitner)
//
opcode_description *opcode_bsearch(const opcode_description *base, unsigned long array_size, const char *key, unsigned long key_length)
{
	unsigned long m;
	int tmp;
	opcode_description *p;

	m = 0UL;
	tmp = 0;
	p = 0;

	while (array_size) {
		m = array_size/2;
		p = (opcode_description *) &base[m];

		tmp = opcode_bsearch_cmp(p, key, key_length);

		if (tmp<0) {
			array_size = m;
		} else if (tmp>0) {
			base = p+1;
			array_size -= m+1;
		} else {
			return p;
		}
	}
	return 0;
}



// ========================================================================================================================================================================
// ========================================================================================================================================================================
// ========================================================================================================================================================================

/// Stackframe for VM
///
/// for C we create a stackframe structure which the stack[] array entries point to...
///
/// The original javascript used an array of "var" segmented into 5 entries each representing the stackframe.
/// This code uses an array of pointers to structures containing the 5 entries.
///
/// for readability we rename variables to:
///
///	gn1  = label_number_A
///	gn2  = label_number_B
///	pc   = frame_ProgramCounter_location
///	rule = rule (still)
///	lm   = left_margin
///
typedef struct stackframe_struct {
	unsigned long 		label_number_A;					///< gn1
	unsigned long 		label_number_B;					///< gn2
	const char *		frame_ProgramCounter_location;	///< pc
	metaII_dynamic_string *	rule;							///< this appears to need garbage collection
	unsigned long 		left_margin;					///< lm
} stackframe;



/// META II Virtual Machine instance
///
/// Contains all globals needed to run an encapsulated META II Virtual Machine
///
typedef struct metaII_vm_struct {

	metaII_memory_allocator *theAllocator;

	/// interpreter information
	/// program counter into the interpreter text
	///
	/// var pc;
	///
	/// not to be confused with a program counter in a bytecode vm - this is the position of the next characted to be lexed/interpreted (imho)
	///
	/// variables renamed for readability:
	///
	///	pc = interpeter_program_ProgramCounter
	///
	const char *interpeter_program_ProgramCounter;

	/// interpreter text
	///
	/// var ic;
	///
	/// variables renamed for readability:
	///
	///	ic = interpreter_program_source_code
	///
	const char *interpreter_program_source_code;

	/// input information
	/// input pointer into the input text
	///
	/// var inp;
	///
	/// variables renamed for readability:
	///
	///	inp = input_current_char
	///
	const char *input_current_char;

	/// input text
	///
	/// var inbuf;
	///
	/// variables renamed for readability:
	///
	///	inbuf = input_source_code
	///
	const char *input_source_code;

	/// output information
	/// output text
	///
	/// var outbuf;
	///
	metaII_dynamic_string *outbuf;

	/// output left margin
	///
	/// var margin;
	///
	/// Code suggests that it can be a negative number
	///
	long int margin;

	/// stack frame pointer into stack array
	///
	/// var stackframe;
	///
	/// for readability we rename variables to:
	///
	///	stackframe = current_stackframe_ptr
	///
	stackframe *current_stackframe_ptr;


	/// runtime variables
	/// interpreter exit flag
	///
	/// var exitlevel;
	///
	/// no "boolean" type expected...
	///
	unsigned long exitlevel;

	/// parser control flag
	///
	/// var flag;
	///
 	/// for readability we rename variables to:
	///
	///	flag = parser_control_flag
	///
	unsigned long parser_control_flag;

	///
	/// argument for order codes with symbol arguments
	///
	/// var symbolarg;
	///
	metaII_dynamic_string *symbolarg;

	/// argument for order codes with string arguments
	///
	/// var stringarg;
	///
	metaII_dynamic_string *stringarg;

	/// next numeric label to use
	///
	/// var gnlabel;
	///
	unsigned long gnlabel;

	/// token string from parse
	///
	/// var token;
	///
	metaII_dynamic_string *token;

	/// output string from code ejection
	///
	/// var outstr;
	///
	metaII_dynamic_string *outstr;


	metaII_dynamic_string *op;
	metaII_dynamic_string *tmpstr;
	metaII_dynamic_string *msg;
	metaII_dynamic_string *ctx;



	/// extension variables
	/// collecting token characters
	///
	/// var tokenflag;
	///
	unsigned long tokenflag;

	int verbose;

	void (*argsymbol)(struct metaII_vm_struct *theVM);
	void (*argstring)(struct metaII_vm_struct *theVM);
	void (*out)(struct metaII_vm_struct *theVM, metaII_dynamic_string *s);
	void (*outchar)(struct metaII_vm_struct *theVM, char s);
	void (*outuint)(struct metaII_vm_struct *theVM, unsigned long i);
	void (*findlabel)(struct metaII_vm_struct *theVM, metaII_dynamic_string *s);
	void (*destroy)(struct metaII_vm_struct *theVM);

	int  (*interpeter)(struct metaII_vm_struct *theVM, const char *interpreter_source, unsigned long interpreter_source_length, const char *input, unsigned long input_length, metaII_dynamic_string *execution_output);



	/// stack of stackframes
	///
	/// stack = new array(600);
	///
	/// for readability we rename variables to:
	///
	///	stack = the_stack
	///
	stackframe the_stack[2000];

} metaII_vm;


void 		metaII_vm_argsymbol(struct metaII_vm_struct *theVM);
void 		metaII_vm_argstring(struct metaII_vm_struct *theVM);
void 		metaII_vm_out(struct metaII_vm_struct *theVM, metaII_dynamic_string *s);
void 		metaII_vm_outchar(struct metaII_vm_struct *theVM, char s);
void 		metaII_vm_outuint(struct metaII_vm_struct *theVM, unsigned long i);
void 		metaII_vm_findlabel(struct metaII_vm_struct *theVM, metaII_dynamic_string *s);
void 		metaII_vm_destroy(struct metaII_vm_struct *theVM);
int  		metaII_vm_interpeter(struct metaII_vm_struct *theVM, const char *interpreter_source, unsigned long interpreter_source_length, const char *input, unsigned long input_length, metaII_dynamic_string *execution_output);
metaII_vm *	metaII_vm_new(metaII_memory_allocator *theAllocator);


metaII_vm *	metaII_vm_new(metaII_memory_allocator *theAllocator)
{
	metaII_vm *newVM;

	newVM = theAllocator->allocate_memory(theAllocator, sizeof(metaII_vm));

	newVM->symbolarg = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->stringarg = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->token = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->outstr = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->op = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->tmpstr = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->msg = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);
	newVM->ctx = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);


	newVM->argsymbol = metaII_vm_argsymbol;
	newVM->argstring = metaII_vm_argstring;
	newVM->out = metaII_vm_out;
	newVM->outchar = metaII_vm_outchar;
	newVM->outuint = metaII_vm_outuint;
	newVM->findlabel = metaII_vm_findlabel;
	newVM->destroy = metaII_vm_destroy;
	newVM->interpeter = metaII_vm_interpeter;

	newVM->theAllocator = theAllocator;
	newVM->verbose = 0;

	return newVM;
}


void metaII_vm_destroy(struct metaII_vm_struct *theVM)
{
	metaII_memory_allocator *theAllocator;

	theAllocator = theVM->theAllocator;

	theVM->symbolarg->destroy(theVM->symbolarg);
	theVM->stringarg->destroy(theVM->stringarg);
	theVM->token->destroy(theVM->token);
	theVM->outstr->destroy(theVM->outstr);
	theVM->op->destroy(theVM->op);
	theVM->tmpstr->destroy(theVM->tmpstr);
	theVM->msg->destroy(theVM->msg);
	theVM->ctx->destroy(theVM->ctx);

	theAllocator->free_memory(theAllocator, (char *) theVM);
}

void metaII_vm_findlabel(struct metaII_vm_struct *theVM, metaII_dynamic_string *s)
{
	theVM->tmpstr->clear(theVM->tmpstr);
	theVM->tmpstr->append(theVM->tmpstr, "\n", 1);
		theVM->tmpstr->append_dyn(theVM->tmpstr, s);
	theVM->tmpstr->append(theVM->tmpstr, "\n", 1);


	theVM->interpeter_program_ProgramCounter = strstr(theVM->interpreter_program_source_code, theVM->tmpstr->ptr);

	if (!theVM->interpeter_program_ProgramCounter) {
		theVM->tmpstr->clear(theVM->tmpstr);
		theVM->tmpstr->append(theVM->tmpstr, "\n", 1);
			theVM->tmpstr->append_dyn(theVM->tmpstr, s);
		theVM->tmpstr->append(theVM->tmpstr, "\r", 1);

		theVM->interpeter_program_ProgramCounter = strstr(theVM->interpreter_program_source_code, theVM->tmpstr->ptr);
	}

	if (theVM->interpeter_program_ProgramCounter) {
		theVM->interpeter_program_ProgramCounter = theVM->interpeter_program_ProgramCounter + s->string_length + 1;
	}

	// notify on unresolved label and stop interpret
	//
	if (!theVM->interpeter_program_ProgramCounter) {
		// window.alert('label '+s+' not found!\n');
		theVM->exitlevel = 1UL;
	}
}



// out - if necessary move to margin before output of s
void metaII_vm_out(struct metaII_vm_struct *theVM, metaII_dynamic_string *s)
{
	long col;

	if ((theVM->margin > 0L) && (theVM->outstr->string_length == 0)) {

		// advance to left margin
		//
		col = 0L;

		while (col < theVM->margin) {
			theVM->outstr->append(theVM->outstr, " ", 1);
			col++;
		}
	}

	// output given string
	theVM->outstr->append_dyn(theVM->outstr, s);
}



void metaII_vm_outchar(struct metaII_vm_struct *theVM, char s)
{
	long col;

	if ((theVM->margin > 0L) && (theVM->outstr->string_length == 0)) {

		// advance to left margin
		//
		col = 0L;

		while (col < theVM->margin) {
			theVM->outstr->append(theVM->outstr, " ", 1);
			col++;
		}
	}

	// output given character
	theVM->outstr->append(theVM->outstr, &s, 1); 
}


void metaII_vm_outuint(struct metaII_vm_struct *theVM, unsigned long i)
{
	long col;
	char buf[128];
	unsigned long len;

	if ((theVM->margin > 0L) && (theVM->outstr->string_length == 0)) {

		// advance to left margin
		//
		col = 0L;

		while (col < theVM->margin) {
			theVM->outstr->append(theVM->outstr, " ", 1);
			col++;
		}
	}

	memset(buf, 0, sizeof(buf));

	len = (unsigned long) snprintf(buf, sizeof(buf) - 1, "%lu", i);

	// output given integer string
	theVM->outstr->append(theVM->outstr, buf, len); 
}


void metaII_vm_argstring(struct metaII_vm_struct *theVM)
{
	theVM->stringarg->clear(theVM->stringarg);

	// find the beginning of the string

	while (*theVM->interpeter_program_ProgramCounter != '\'') {
		theVM->interpeter_program_ProgramCounter++;
	}

	// concat string together
	theVM->interpeter_program_ProgramCounter++;

	while (*theVM->interpeter_program_ProgramCounter != '\'') {
		theVM->stringarg->append(theVM->stringarg, theVM->interpeter_program_ProgramCounter, 1);
		theVM->interpeter_program_ProgramCounter++;
	}

	// skip terminating single quote
	theVM->interpeter_program_ProgramCounter++;
}

void metaII_vm_argsymbol(struct metaII_vm_struct *theVM)
{
	// reset symbol
	theVM->symbolarg->clear(theVM->symbolarg);

	// skip over the operator (not tab and not blank)

	while ((*theVM->interpeter_program_ProgramCounter != ' ') && (*theVM->interpeter_program_ProgramCounter != '\t')) {
		theVM->interpeter_program_ProgramCounter++;
	}

	// skip over tabs or blanks
	while ((*theVM->interpeter_program_ProgramCounter == ' ') || (*theVM->interpeter_program_ProgramCounter == '\t')) {
		theVM->interpeter_program_ProgramCounter++;
	}

	// accrete symbol of alpha and numeral
	while ( ((*theVM->interpeter_program_ProgramCounter >= 'A') && (*theVM->interpeter_program_ProgramCounter <= 'Z')) ||
	        ((*theVM->interpeter_program_ProgramCounter >= '0') && (*theVM->interpeter_program_ProgramCounter <= '9')) ) {
		theVM->symbolarg->append(theVM->symbolarg, theVM->interpeter_program_ProgramCounter, 1);
		theVM->interpeter_program_ProgramCounter++;
	}
}




#define opcode_ADR		0
#define opcode_B		1
#define opcode_BT		2
#define opcode_BF		3
#define opcode_BE		4
#define opcode_CLL		5
#define opcode_CL		6
#define opcode_CI		7
#define opcode_END		8
#define opcode_GN1		9
#define opcode_GN2		10
#define opcode_ID		11
#define opcode_LB		12
#define opcode_NUM		13
#define opcode_OUT		14
#define opcode_R		15
#define opcode_SET		16
#define opcode_SR		17
#define opcode_TST		18
#define opcode_GN		19
#define opcode_LMI		20
#define opcode_LMD		21
#define opcode_NL		22
#define opcode_TB		23
#define opcode_CE		24
#define opcode_CGE		25
#define opcode_CLE		26
#define opcode_LCH		27
#define opcode_NOT		28
#define opcode_RF		29
#define opcode_SCN		30
#define opcode_TFF		31
#define opcode_TFT		32
#define opcode_PFF		33
#define opcode_PFT		34
#define opcode_CC		35
#define opcode_UNKNOWN	36
#define max_opcode_array_size (opcode_UNKNOWN + 1)



int metaII_vm_interpeter(struct metaII_vm_struct *theVM, const char *interpreter_source, unsigned long interpreter_source_length, const char *input, unsigned long input_length, metaII_dynamic_string *execution_output)
{
	static opcode_description opcode_descriptions[max_opcode_array_size] = {

		// entries sorted for bsearch()
		//
		{opcode_ADR, 		"ADR",	3UL, "specify starting rule", (void *) 0 },
		{opcode_B, 			"B",	1UL, "unconditional branch to label", (void *) 0 },
		{opcode_BE, 		"BE",	2UL, "branch if switch false to error halt", (void *) 0 },
		{opcode_BF, 		"BF",	2UL, "branch if switch false to label", (void *) 0 },
		{opcode_BT, 		"BT",	2UL, "branch if switch true to label", (void *) 0 },
		{opcode_CC, 		"CC",	2UL, "copy char code to output", (void *) 0 },
		{opcode_CE, 		"CE",	2UL, "compare input char to code for equal", (void *) 0 },
		{opcode_CGE, 		"CGE",	3UL, "compare input char to code for greater or equal", (void *) 0 },
		{opcode_CI, 		"CI",	2UL, "copy scanned token to output", (void *) 0 },
		{opcode_CL, 		"CL",	2UL, "copy given string argument to output", (void *) 0 },
		{opcode_CLE, 		"CLE",	3UL, "compare input char to code for less or equal", (void *) 0 },
		{opcode_CLL, 		"CLL",	3UL, "call rule at label", (void *) 0 },
		{opcode_END, 		"END",	3UL, "pseudo op, end of source", (void *) 0 },
		{opcode_GN1, 		"GN1",	3UL, "make and output label 1", (void *) 0 },
		{opcode_GN2, 		"GN2",	3UL, "make and output label 2", (void *) 0 },
		{opcode_GN, 		"GN",	2UL, "make and output unique number", (void *) 0 },
		{opcode_ID, 		"ID",	2UL, "recognize identifier token", (void *) 0 },
		{opcode_LB, 		"LB",	2UL, "start output in label field", (void *) 0 },
		{opcode_LCH, 		"LCH",	3UL, "literal character code to token as string", (void *) 0 },
		{opcode_LMD, 		"LMD",	3UL, "left margin decrease", (void *) 0 },
		{opcode_LMI, 		"LMI",	3UL, "left margin increase", (void *) 0 },
		{opcode_NL, 		"NL",	2UL, "new line output", (void *) 0 },
		{opcode_NOT, 		"NOT",	3UL, "complement flag", (void *) 0 },
		{opcode_NUM, 		"NUM",	3UL, "recognize number token", (void *) 0 },
		{opcode_OUT, 		"OUT",	3UL, "output out buffer with new line", (void *) 0 },
		{opcode_PFF, 		"PFF",	3UL, "parse flag set to false", (void *) 0 },
		{opcode_PFT, 		"PFT",	3UL, "parse flag set to true (AKA SET)", (void *) 0 },
		{opcode_RF, 		"RF",	2UL, "return if switch false", (void *) 0 },
		{opcode_R, 			"R",	1UL, "return from rule call with CLL", (void *) 0 },
		{opcode_SCN, 		"SCN",	3UL, "if flag, scan input character; if token flag, add to token", (void *) 0 },
		{opcode_SET, 		"SET",	3UL, "set switch true", (void *) 0 },
		{opcode_SR, 		"SR",	2UL, "recognize string token including single quotes", (void *) 0 },
		{opcode_TB, 		"TB",	2UL, "output a tab", (void *) 0 },
		{opcode_TFF, 		"TFF",	3UL, "token flag set to false", (void *) 0 },
		{opcode_TFT, 		"TFT",	3UL, "token flag set to true", (void *) 0 },
		{opcode_TST, 		"TST",	3UL, "test for given string argument, if found set switch", (void *) 0 },
		{opcode_UNKNOWN, 	0, 		0,   "unknown token", (void *) 0 }

	};

	metaII_memory_allocator *theAllocator;
	const char *oc;
	unsigned long oc_index;
	opcode_description *opcode_description_ptr;
	
	unsigned long current_opcode_value;

	theAllocator = theVM->theAllocator;

	theVM->input_source_code 					= input;
	theVM->interpreter_program_source_code 		= interpreter_source;
	theVM->outbuf 								= execution_output;


	theVM->interpeter_program_ProgramCounter 	= theVM->interpreter_program_source_code;
	theVM->outstr->append(theVM->outstr, "\t", 1);
	theVM->exitlevel = 0UL;


	while (!theVM->exitlevel) {



		// assumes pc on operator in line
		oc = theVM->interpeter_program_ProgramCounter;
		oc_index = (unsigned long) (oc - theVM->interpreter_program_source_code);
		theVM->op->clear(theVM->op);

		// accrete operator of upper alpha and numeral
		//
		while ( (oc_index < interpreter_source_length) && (*theVM->interpeter_program_ProgramCounter != '\t') ) {
			oc++;
			oc_index++;
			theVM->interpeter_program_ProgramCounter++;
		}

		if (!(oc_index < interpreter_source_length)) {
			theVM->exitlevel = 1UL;
			if (theVM->verbose) { fprintf(stderr, "scanned pass EOF - done\n"); }
			continue;
		}

		theVM->interpeter_program_ProgramCounter++;

			// This block is based on the original Javascript InterpretOp() method
			// The various other methods have been condensed into the switch/case to avoid additional function calls
			// This was also done to support jump-threading in the future
			//
			//*****//


				// assumes pc on operator in line
				oc = theVM->interpeter_program_ProgramCounter;
				oc_index = (unsigned long) (oc - theVM->interpreter_program_source_code);
				theVM->op->clear(theVM->op);

				// accrete operator of upper alpha and numeral
				//
				while ( (oc_index < interpreter_source_length) &&
						(((*oc >= 'A') && (*oc <= 'Z')) ||
						 ((*oc >= '0') && (*oc <= '9'))) ) {
					theVM->op->append(theVM->op, oc, 1);
					oc++;
					oc_index++;
				}

				if (!(oc_index < interpreter_source_length)) {
					theVM->exitlevel = 1UL;
					if (theVM->verbose) { fprintf(stderr, "scanned pass EOF - done\n"); }
					continue;
				}



				opcode_description_ptr = opcode_bsearch((const opcode_description *) &(opcode_descriptions[0]), max_opcode_array_size, theVM->op->ptr, theVM->op->string_length);

				if (opcode_description_ptr) {
					current_opcode_value = opcode_description_ptr->opcode_value;
if (theVM->verbose) { fprintf(stderr, "-->%s<--\n", opcode_description_ptr->opcode_string); }
				} else { 
					current_opcode_value = opcode_UNKNOWN;
				}


				// intrepreter op case branch
				//
				// The original javascript used a switch() with a string argument and string cases.
				// To emulate this, we use an array of opcodes and bsearch() them to retrieve an opcode_value which then
				// use as the argument to our switch/case statement.  
				//  
				switch (current_opcode_value) {
					//
					// original META II order codes

					case opcode_ADR:  // ADR - specify starting rule
						theVM->argsymbol(theVM);
						theVM->gnlabel = 1UL;
						theVM->input_current_char = theVM->input_source_code;  //dcc// bootstrap input_current_char pointer to input_source_code pointer
						theVM->margin = 0L;
						theVM->current_stackframe_ptr = theVM->the_stack;
						// initialize first stackframe
						theVM->current_stackframe_ptr->label_number_A = 0UL;         // GN1  also GN (extended only)
						theVM->current_stackframe_ptr->label_number_B = 0UL;         // GN2
						theVM->current_stackframe_ptr->frame_ProgramCounter_location = (char *) 0;        // return pc value
						theVM->current_stackframe_ptr->rule = theVM->symbolarg->duplicate(theVM->symbolarg); // rule name called for error messages (we make a copy for the frame)
						theVM->current_stackframe_ptr->left_margin = theVM->margin;    // left margin (extended only)
						theVM->findlabel(theVM,theVM->symbolarg);

						break;

					case opcode_B: // B   - unconditional branch to label
						theVM->argsymbol(theVM);
						theVM->findlabel(theVM,theVM->symbolarg);
						break; 

					case opcode_BT: // BT  - branch if switch true to label
						theVM->argsymbol(theVM);

						if (theVM->parser_control_flag) {
							theVM->findlabel(theVM,theVM->symbolarg);
						}

						break; 

					case opcode_BF: // BF  - branch if switch false to label
						theVM->argsymbol(theVM);

						if (! theVM->parser_control_flag) {
							theVM->findlabel(theVM,theVM->symbolarg);
						}

						break; 

					case opcode_BE: // BE  - branch if switch false to error halt
						{
							const char *i;
							const char *j;
							unsigned long h;

							// only halt if there is an error

							if (theVM->parser_control_flag) {
								break; // switch(op)
							}


							// provide error context
							//
							theVM->msg->clear(theVM->msg);

							theVM->msg->append(theVM->msg, "SYNTAX ERROR:\nrule:", 19);
							theVM->msg->append_dyn(theVM->msg, theVM->current_stackframe_ptr->rule);
							theVM->msg->append(theVM->msg, "\nlast token:", 12);
							theVM->msg->append_dyn(theVM->msg, theVM->token);
							theVM->msg->append(theVM->msg, "\nout string:", 12);
							theVM->msg->append_dyn(theVM->msg, theVM->outstr);
							theVM->msg->append(theVM->msg, "\nINPUT:\n", 8);


							// provide scan context
							if ((theVM->input_current_char - theVM->input_source_code) < 20) {
								i = theVM->input_source_code;
							} else {
								i = theVM->input_current_char - 20;
							}

							if (((theVM->input_current_char + 20) - theVM->input_source_code) > input_length) {
								j = theVM->input_source_code + input_length;
							} else {
								j = theVM->input_current_char + 20;
							}


							theVM->ctx->clear(theVM->ctx);

							theVM->ctx->append(theVM->ctx, i, (unsigned long) (theVM->input_current_char - i) );
							theVM->ctx->append(theVM->ctx, "<scan>", 6 );
							theVM->ctx->append(theVM->ctx, theVM->input_current_char, (unsigned long) (j - theVM->input_current_char) );


							theVM->msg->append_dyn(theVM->msg, theVM->ctx);
							theVM->msg->append(theVM->msg, "\n\nCHAR CODES:\n", 14);

							// ensure all character codes are visible

							for (h = 0; h < theVM->ctx->string_length; h++) {
								if (theVM->ctx->ptr[h] <= 32) {
									char buf[16];
									unsigned long len;
									memset(buf, 0, sizeof(buf));
									len = snprintf(buf, sizeof(buf)-1, "<%i>", theVM->ctx->ptr[h]);
									theVM->msg->append(theVM->msg, buf, len);
								} else {
									theVM->msg->append(theVM->msg, (char *) (theVM->ctx->ptr + h), 1);
								}
							}

							theVM->msg->append(theVM->msg, "\n", 1);

							fprintf(stderr, "%s", theVM->msg->ptr);

							theVM->exitlevel = 1UL;
						}
						break;  

					case opcode_CLL: // CLL - call rule at label
						theVM->argsymbol(theVM);

						// push and initialize a new stackframe
						theVM->current_stackframe_ptr++;

						theVM->current_stackframe_ptr->label_number_A = 0UL;         // GN1  also GN (extended only)

						theVM->current_stackframe_ptr->label_number_B = 0UL;         // GN2

						theVM->current_stackframe_ptr->frame_ProgramCounter_location = theVM->interpeter_program_ProgramCounter;        // return pc value

						theVM->current_stackframe_ptr->rule = theVM->symbolarg->duplicate(theVM->symbolarg); // rule name called for error messages

						theVM->current_stackframe_ptr->left_margin = theVM->margin;    // left margin (needed on backtrack)

						theVM->findlabel(theVM,theVM->symbolarg);

						break; 

					case opcode_CL: // CL  - copy given string argument to output
						theVM->argstring(theVM);

						theVM->out(theVM, theVM->stringarg);

						break; 

					case opcode_CI: // CI  - copy scanned token to output
						theVM->out(theVM, theVM->token);

						break;  

					case opcode_END: // END - pseudo op, end of source
						theVM->exitlevel = 1UL;

						if (!theVM->parser_control_flag) {
							// window.alert('first rule "'+ theVM->current_stackframe_ptr->rule + '" failed');  // TODO
						}


						break; 

					case opcode_GN1: // GN1 - make and output label 1
						if (theVM->current_stackframe_ptr->label_number_A == 0UL) {
							theVM->current_stackframe_ptr->label_number_A = theVM->gnlabel;
							theVM->gnlabel++;
						}

						theVM->outchar(theVM, 'L');
						theVM->outuint(theVM, theVM->current_stackframe_ptr->label_number_A);

						break; 

					case opcode_GN2: // GN2 - make and output label 2
						if (theVM->current_stackframe_ptr->label_number_B == 0UL) {
							theVM->current_stackframe_ptr->label_number_B = theVM->gnlabel;
							theVM->gnlabel++;
						}


						theVM->outchar(theVM, 'B');
						theVM->outuint(theVM, theVM->current_stackframe_ptr->label_number_B);

						break; 

					case opcode_ID: // ID  - recognize identifier token
						// walk over whitespace
						//
						while (	(*theVM->input_current_char == ' ') || (*theVM->input_current_char == '\n') || (*theVM->input_current_char == '\r') || (*theVM->input_current_char == '\t') ) { theVM->input_current_char++; }

						// accept upper alpha or lower alpha
						theVM->parser_control_flag = ( ((*theVM->input_current_char >= 'A') && (*theVM->input_current_char <= 'Z')) ||
								 ((*theVM->input_current_char >= 'a') && (*theVM->input_current_char <= 'z')) );

						if (theVM->parser_control_flag) {
							theVM->token->clear(theVM->token);

							while (theVM->parser_control_flag) {
								// add to token
								theVM->token->append(theVM->token, theVM->input_current_char, 1);
								theVM->input_current_char++;

								// accept upper alpha or lower alpha or numeral
								theVM->parser_control_flag = ( ((*theVM->input_current_char >= 'A') && (*theVM->input_current_char <= 'Z')) ||
										 ((*theVM->input_current_char >= 'a') && (*theVM->input_current_char <= 'z')) ||
										 ((*theVM->input_current_char >= '0') && (*theVM->input_current_char <= '9')) );
							}

							theVM->parser_control_flag = 1UL;
						}

						break;  

					case opcode_LB: // LB  - start output in label field
						theVM->outstr->clear(theVM->outstr);

						break;  

					case opcode_NUM: // NUM - recognize number token
						// walk over whitespace
						//
						while (	(*theVM->input_current_char == ' ') || (*theVM->input_current_char == '\n') || (*theVM->input_current_char == '\r') || (*theVM->input_current_char == '\t') ) { theVM->input_current_char++; }

						// accept a numeral
						theVM->parser_control_flag = ((*theVM->input_current_char >= '0') && (*theVM->input_current_char <= '9'));

						if (theVM->parser_control_flag) {
							theVM->token->clear(theVM->token);

							while (theVM->parser_control_flag) {
								// add to token
								theVM->token->append(theVM->token, theVM->input_current_char, 1);
								theVM->input_current_char++;

								// accept numerals
								theVM->parser_control_flag = ((*theVM->input_current_char >= '0') && (*theVM->input_current_char <= '9'));
							}

							theVM->parser_control_flag = 1UL;
						}

						break; 

					case opcode_OUT: // OUT - output out buffer with new line

//theVM->outstr->log(theVM->outstr, "outstr contents");

						theVM->outbuf->append_dyn(theVM->outbuf, theVM->outstr);
						theVM->outbuf->append(theVM->outbuf, "\n", 1); 

//theVM->outbuf->log(theVM->outbuf, "OUT Exec");

						theVM->outstr->clear(theVM->outstr);
						theVM->outstr->append(theVM->outstr, "\t", 1);

						break; 

					case opcode_R: // R   - return from rule call with CLL
						// interpretation completed on return on empty stack
						if (theVM->current_stackframe_ptr == theVM->the_stack) {
							theVM->exitlevel = 1UL;

							if (!theVM->parser_control_flag) {
								// window.alert('first rule "'+ theVM->current_stackframe_ptr->rule + '" failed'); // TODO
							}

							break; // switch(op)
						}

						// get return pc from stackframe and pop stack
						theVM->interpeter_program_ProgramCounter = theVM->current_stackframe_ptr->frame_ProgramCounter_location; // return pc

						theVM->margin = theVM->current_stackframe_ptr->left_margin;

						// free the copy of the rule...
						//
						theVM->current_stackframe_ptr->rule->destroy(theVM->current_stackframe_ptr->rule);
						theVM->current_stackframe_ptr->rule = 0;

						theVM->current_stackframe_ptr--;                                // pop stackframe

						break;   

					case opcode_SET: // SET - set switch true
						theVM->parser_control_flag = 1UL;

						break; 

					case opcode_SR: // SR  - recognize string token including single quotes
						// walk over whitespace
						//
						while (	(*theVM->input_current_char == ' ') || (*theVM->input_current_char == '\n') || (*theVM->input_current_char == '\r') || (*theVM->input_current_char == '\t') ) { theVM->input_current_char++; }

						// accept a single quote
						theVM->parser_control_flag = (*theVM->input_current_char == '\'');

						if (theVM->parser_control_flag) {
							theVM->token->clear(theVM->token);

							while (theVM->parser_control_flag) {
								// add to token
								theVM->token->append(theVM->token, theVM->input_current_char, 1);
								theVM->input_current_char++;

								// accept anything but a single quote
								theVM->parser_control_flag = (*theVM->input_current_char != '\'');
							}

							// skip teminating single quote
							theVM->token->append(theVM->token, "'", 1);

							theVM->input_current_char++;

							theVM->parser_control_flag = 1UL;
						}

						break;  

					case opcode_TST: // TST - test for given string argument, if found set switch
						{
							unsigned long i;

							theVM->argstring(theVM);

							// walk over whitespace
							//
							while (	(*theVM->input_current_char == ' ') || (*theVM->input_current_char == '\n') || (*theVM->input_current_char == '\r') || (*theVM->input_current_char == '\t') ) { theVM->input_current_char++; }

							// test string case insensitive
							theVM->parser_control_flag = 1UL;

							i = 0UL;

							while (theVM->parser_control_flag && (i < theVM->stringarg->string_length) ) {
								theVM->parser_control_flag = ((toupper(*(theVM->stringarg->ptr+i)) == toupper(*(theVM->input_current_char+i))) ? 1UL : 0UL);
								i++;
							}

							// advance input if found
							if (theVM->parser_control_flag) {
								theVM->input_current_char += theVM->stringarg->string_length;
							}
						}
						break; 

						// extensions to provide label and nested output definition

					case opcode_GN: // GN  - make and output unique number
						if (theVM->current_stackframe_ptr->label_number_A == 0UL) {
							theVM->current_stackframe_ptr->label_number_A = theVM->gnlabel;
							theVM->gnlabel++;
						}

						theVM->outuint(theVM, theVM->current_stackframe_ptr->label_number_A);

						break; 

					case opcode_LMI: // LMI - left margin increase
						theVM->margin += 2L;

						break; 

					case opcode_LMD: // LMD - left margin decrease
						theVM->margin -= 2L;

						break; 

					case opcode_NL: // NL  - new line output

//theVM->outstr->log(theVM->outstr, "outstr contents");

						theVM->outbuf->append_dyn(theVM->outbuf, theVM->outstr);
						theVM->outbuf->append(theVM->outbuf, "\n", 1);


//theVM->outbuf->log(theVM->outbuf, "NL Exec");

						theVM->outstr->clear(theVM->outstr);

						break; 

					case opcode_TB: // TB  - output a tab
						theVM->outchar(theVM, '\t');

						break; 

						// extensions to provide token definition

					case opcode_CE: // CE  - compare input char to code for equal
						theVM->argsymbol(theVM);
						theVM->parser_control_flag = ((*theVM->input_current_char == *(theVM->symbolarg->ptr)) ? 1 : 0);

						break; 

					case opcode_CGE: // CGE - compare input char to code for greater or equal
						theVM->argsymbol(theVM);
						theVM->parser_control_flag = ((*theVM->input_current_char >= *(theVM->symbolarg->ptr)) ? 1 : 0);  // TODO what is symbol arg pointing at and is it an integer or string?

						break;

					case opcode_CLE: // CLE - compare input char to code for less or equal
						theVM->argsymbol(theVM);
						theVM->parser_control_flag = ((*theVM->input_current_char <= *(theVM->symbolarg->ptr)) ? 1 : 0); // TODO what is symbol arg pointing at and is it an integer or string?

						break; 

					case opcode_LCH: // LCH - literal character code to token as string
						theVM->token->clear(theVM->token);
						theVM->token->append(theVM->token, theVM->input_current_char, 1);

						// scan the character
						theVM->input_current_char++;

						break; 

					case opcode_NOT: // NOT - complement flag
						theVM->parser_control_flag = !theVM->parser_control_flag;

						break; 

					case opcode_RF: // RF  - return if switch false
						if (!theVM->parser_control_flag) {

							// runR
							//
							// interpretation completed on return on empty stack
							if (theVM->current_stackframe_ptr == theVM->the_stack) {
								theVM->exitlevel = 1UL;

								if (!theVM->parser_control_flag) {
									// window.alert('first rule "'+ theVM->current_stackframe_ptr->rule + '" failed'); // TODO
								}

								break; // switch(op)
							}

							// get return pc from stackframe and pop stack
							theVM->interpeter_program_ProgramCounter = theVM->current_stackframe_ptr->frame_ProgramCounter_location; // return pc

							theVM->margin = theVM->current_stackframe_ptr->left_margin;

							// free the copy of the rule...
							//
							theVM->current_stackframe_ptr->rule->destroy(theVM->current_stackframe_ptr->rule);
							theVM->current_stackframe_ptr->rule = 0;

							theVM->current_stackframe_ptr--;                                // pop stackframe
						}

						break; 

					case opcode_SCN: // SCN - if flag, scan input character; if token flag, add to token
						if (theVM->parser_control_flag) {

							// if taking token, add to token
							if (theVM->tokenflag) {
								theVM->token->append(theVM->token, theVM->input_current_char, 1);
							}

							// scan the character
							theVM->input_current_char++;
						}

						break; 

					case opcode_TFF: // TFF - token flag set to false
						theVM->tokenflag = 0UL;

						break; 

					case opcode_TFT: // TFT - set token flag true and clear token
						theVM->tokenflag = 1UL;

						theVM->token->clear(theVM->token);

						break; 

						// extensions for backtracking, error handling, and char code output

					case opcode_PFF: // PFF - parse flag set to false
						theVM->parser_control_flag = 0UL;

						break; 

					case opcode_PFT: // PFT - parse flag set to true (AKA SET)
						theVM->parser_control_flag = 1UL;

						break; 

					case opcode_CC: // CC - copy char code to output 
						{
							unsigned char theChar;
							unsigned long theValue;

							theVM->argsymbol(theVM);
						
							theValue = strtoul(theVM->symbolarg->ptr, 0, 10);
							theChar = (unsigned char) theValue; // taking to low byte....
	
							theVM->outstr->append(theVM->outstr, &theChar, 1);
						}


						break; 

					case opcode_UNKNOWN:
					default:
						//window.alert('ERROR: unknown interpret op \''+op+'\''); // TODO

						theVM->exitlevel = 1UL;

						break;

				} // switch

			//*****// block from InterpretOp()

	} // while (!theVM->exitlevel)


	return 1;
}





// --------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------
//   ****** "4 days...  4 days from first reading about META II on Mr. Neighbors website to this.... lol" ************ -dcc
// --------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------


// TODO use i03 input to bootstrap interpeter for Syntax Output

// global arrays
// way too big, but that is ok...
//
meta_compiler_workshop_sources metaII_testkit_mc_workshop_examples_Inputs[64];
meta_compiler_workshop_sources metaII_testkit_mc_workshop_examples_Codes[64];

void metaII_testkit_mc_workshop_examples_init(int *maxInput, int *maxCode)
{
	get_meta_compiler_workshop_sources_for_Input(&(metaII_testkit_mc_workshop_examples_Inputs[0]), maxInput);
	get_meta_compiler_workshop_sources_for_Code(&(metaII_testkit_mc_workshop_examples_Codes[0]), maxCode);
}

int metaII_testkit_mc_workshop_examples(int inputIndex, int codeIndex, metaII_memory_allocator *theAllocator, int verbose)
{
	metaII_vm *theVM;
	metaII_dynamic_string *theOutput;
	const char *interpreter_source;
	unsigned long interpreter_source_length;
	const char *input;
	unsigned long input_length;



	input = metaII_testkit_mc_workshop_examples_Inputs[inputIndex].text;
	interpreter_source = metaII_testkit_mc_workshop_examples_Codes[codeIndex].text;



	interpreter_source_length = (unsigned long) strlen(interpreter_source);
	input_length = (unsigned long) strlen(input);



	theOutput = (metaII_dynamic_string *) metaII_dynamic_string_new(theAllocator);



		theVM = (metaII_vm *) metaII_vm_new(theAllocator);
		
		theVM->verbose = verbose;


			theVM->interpeter(theVM, interpreter_source, interpreter_source_length, input, input_length, theOutput);



	fwrite(theOutput->ptr, 1, theOutput->string_length, stdout);

	theVM->destroy(theVM);

	theOutput->destroy(theOutput);

	return 0;
}

void help(char *s) {
	fprintf(stderr, "usage: %s [-i #] [-c #] \n",s);
	fprintf(stderr, "-i #\t\tspecify example input text index from mc_workshop (i01..i0X)\n");
	fprintf(stderr, "-c #\t\tspecify example code index from mc_workshop       (c01..i0X)\n");
	fprintf(stderr, "-v  \t\tverbose mode\n");
	fprintf(stderr, "-l  \t\tlist example input and code indexes\n");
//	fprintf(stderr, "-I filename\tspecify input text to be loaded from filename\n");
//	fprintf(stderr, "-C filename\tspecify code to be loaded from filename\n");
//	fprintf(stderr, "-O filename\tspecify output to be written to filename\n");
}

int main(int argc, char **argv)
{
	int return_code;
	int ch;
	int i;
	int userInputIndex;
	int userCodeIndex;
//	char *userInputFileName;
//	char *userCodeFileName;
//	char *userOutputFileName;
	int verbose;
	int listExamples;
	int maxInput;
	int maxCode;
	metaII_memory_allocator *theAllocator;

	return_code = 0;

	maxInput = 0;
	maxCode = 0;
	userInputIndex = -1;
	userCodeIndex = -1;
	verbose = 0;
	listExamples = 0;
//	userInputFileName = 0;
//	userCodeFileName = 0;
//	userOutputFileName = 0;


	if (argc < 2) {
		help(argv[0]);
	}

	metaII_testkit_mc_workshop_examples_init(&maxInput, &maxCode);

	// I:C:O:
	while ((ch = getopt(argc, argv, "i:c:vl")) != -1) {
		switch (ch) {
			case 'i':				userInputIndex = atoi(optarg);	
									break;
			case 'c':				userCodeIndex = atoi(optarg);		
									break;
			case 'v':				verbose = 1;
									break;

			case 'l':				listExamples = 1;
									break;

//			case 'I':				userInputFileName = optarg;
//									break;
//
//			case 'C':				userCodeFileName = optarg;
//									break;
//
//			case 'O':				userOutputFileName = optarg;
//									break;

			default:
			case '?':				help(argv[0]);
									exit(1);
		}
	}

	if (listExamples) {

		fprintf(stderr, "\nAvailable Input Texts Are:\n");

		for (i = 0; i<maxInput; i++) {
			fprintf(stderr, "\t%i\t%s\n", i, metaII_testkit_mc_workshop_examples_Inputs[i].menu);
		}

		fprintf(stderr, "\n\nAvailable Codes Are:\n");

		for (i = 0; i<maxCode; i++) {
			fprintf(stderr, "\t%i\t%s\n", i, metaII_testkit_mc_workshop_examples_Codes[i].menu);
		}

		fprintf(stderr, "\n\n");

		exit(1);
	}

	if ((userInputIndex == -1) || (userCodeIndex == -1)) {
		fprintf(stderr, "You must supply an input text index number and a code index number... ( -i 1 -c 1 )\n");
		exit(1);
	}

	if ((userInputIndex > maxInput) || (userCodeIndex > maxCode)) {
		fprintf(stderr, "One of your inputs is invalid...\n");
		fprintf(stderr, "Your Input Index is: %i of a maximum of %i\n", userInputIndex, maxInput);
		fprintf(stderr, "Your Code Index is : %i of a maximum of %i\n", userCodeIndex, maxCode);
		exit(1);
	}


	if (verbose) {
		fprintf(stderr, "You selected input text %i and code %i\n", userInputIndex, userCodeIndex);
		fprintf(stderr, "Input Name: %s\n", metaII_testkit_mc_workshop_examples_Inputs[userInputIndex].menu);
		fprintf(stderr, "Code Name : %s\n", metaII_testkit_mc_workshop_examples_Codes[userCodeIndex].menu);
	}



	theAllocator = (metaII_memory_allocator *) metaII_memory_allocator_new();


		// run the example...
		//
		return_code = metaII_testkit_mc_workshop_examples(userInputIndex, userCodeIndex, theAllocator, verbose);


	theAllocator->destroy(theAllocator);


 return return_code;
}








/*source_file *source_file_load_file(const char *input_name, simple_memory_allocator *allocator)
{
	FILE *input;
	size_t input_name_length;
	source_file *theFile;
	source_line *theLine;
	size_t line_buffer_length;
	char line_buffer[2048];

fprintf(stderr, "load [%s]\n",input_name);

	input_name_length = strlen(input_name);
fprintf(stderr, "load length [%d]\n",input_name_length);

	if (input_name_length == 1) {

		if (*input_name == '-') {
			input = stdin;
fprintf(stderr, "load [stdin]\n");
		}

fprintf(stderr, "----\n");
	
	} else {
fprintf(stderr, "loading [%s] - input = 0\n",input_name);
		input = (FILE *) 0;
	}


	if (!input) {
		input = fopen(input_name, "r");
	}
	

	theFile = source_file_new(allocator);

	// clear the whole thing
	//
	memset(line_buffer, 0, sizeof(line_buffer));
	
	while (fgets(line_buffer, sizeof(line_buffer) - 1, input)) {
	
		line_buffer_length = strnlen(line_buffer, sizeof(line_buffer) - 1);

		if (line_buffer_length) {

			theLine = theFile->append_line(theFile, line_buffer, line_buffer_length);
			if (!theLine) {
				break;
			}
//fprintf(stderr, "%03d >>> ", line_buffer_length);
//theLine->output(theLine, stderr);

		} else {
			break;
		}

		// clear just what we used...
		//
		memset(line_buffer, 0,  line_buffer_length + 1 );
	}

	return theFile;
}


unsigned long source_file_write_file(struct source_file_struct *theFile, const char *output_name)
{
	FILE *output;
	size_t output_name_length;


	output_name_length = strlen(output_name);

	if (output_name_length == 1) {

		if (*output_name == '-') {
			output = stdout;
		}
	
	} else {
		output = (FILE *) 0;
	}

	if (!output) {
		output = fopen(output_name, "w");
	}
	
	
	theFile->output(theFile, output);

	fflush(output);
	fclose(output);
}

*/


