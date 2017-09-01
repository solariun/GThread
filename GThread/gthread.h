/*---------------------------------------
 * gthread - poweful user thread made simple
 *
 * Programmer: Gustavo Campos
 * 
 * Date : 22-08-2017
 *
 * HEADER
 
     MIT License
     
     Copyright (c) 2017 Luiz Gustavo de Campos
     
     Permission is hereby granted, free of charge, to any person obtaining a copy
     of this software and associated documentation files (the "Software"), to deal
     in the Software without restriction, including without limitation the rights
     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     copies of the Software, and to permit persons to whom the Software is
     furnished to do so, subject to the following conditions:
     
     The above copyright notice and this permission notice shall be included in all
     copies or substantial portions of the Software.
     
     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
     SOFTWARE.
     
 * -------------------------------------
 */

#define _XOPEN_SOURCE 600
#define _DARWIN_C_SOURCE 600


#include <unistd.h>
#include <stdio.h>
#include <ucontext.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include <vector>

using namespace std;


#ifndef uint
typedef unsigned int uint;
#endif


typedef struct
{
    ucontext_t 	uContext;
    uint8_t 	nThreadStatus;
    
    uint64_t    nStartTime;
    uint64_t    nSleepETA;

    /* select */
    int          nFD;
    fd_set*      fdSetList [3];
    int          select_ret;
    int          Errorno;
    
    uint16_t	nPriority;
    const char* pstrThreadName;

    uint64_t    nNextRunningTick;
    bool        boolToRun = false;
} thread;


class gthread 
{
public:
	gthread();

	void CreateThread(void (*pFunction)(), uint16_t nPriority, const char* pstrName, uint nArgs, void** ppArgs);

	void CreateThread(void (*pFunction)(), uint64_t nStack, uint16_t nPriority, const char* pstrName, uint nArgs, void** ppArgs);
	
    /* Context information */
    
    const char* getCurrentThreadName();

    void Start ();

    bool Microleep(uint64_t nuTime);

    bool Continue();
    
    const uint64_t getCurrentTick();
    
    int select (int nfds, fd_set *readfds, fd_set *writefds,
         fd_set *errorfds, struct timeval *timeout);
    
    const char* getFDSETErrorMessage();
    
private:

    

    uint64_t nTick = 41;

	vector<thread*> vecThread;

    thread threadStart;

	thread threadKernel;
	
    thread *pThreadWorking = NULL;
	
    uint64_t markNextRunningContexts(uint64_t nTick);

    void sortPriorityDescendently(vector<thread*> &vec);

    void firstCallFunction (void (*pFunction)(), thread *pThread);


protected:

	
    uint64_t getTimeInMicroSec (void);
    
    
    void _verify(bool boolExpression, const char* pstrCode, const char* pstrFunction, const char* pstrFileName, const int nLineNumber, const char* pstrMessage, ...);
} Kernel;

