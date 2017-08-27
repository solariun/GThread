/*---------------------------------------
 * gthread - poweful user thread made simple
 *
 * Programmer: Gustavo Campos
 * 
 * Date : 22-08-2017
 *
 * HEADER
 
     MIT License
     
     Copyright (c) [year] [fullname]
     
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

#include <vector>

using namespace std;


#ifndef uint
typedef unsigned int uint;
#endif


typedef struct
{
    ucontext_t 	uContext;
    uint8_t 	nThreadStatus;
    
    useconds_t	nSleepTime;
    uint64_t    nStartTime;

    uint16_t	nPriority;
    const char* pstrThreadName;

    useconds_t  nNextRunningTick;
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

    bool USleep(useconds_t nuTime);

    bool Continue();
    
    const useconds_t getCurrentTick();
    
private:

    

    useconds_t nTick = 41;

	vector<thread*> vecThread;

    thread threadStart;

	thread threadKernel;
	
    thread *pThreadWorking = NULL;
	
    useconds_t markNextRunningContexts(useconds_t nTick);

    void sortPriorityDescendently(vector<thread*> &vec);

    void firstCallFunction (void (*pFunction)(), thread *pThread);


protected:

	uint64_t getTimeInNanoSec (void);

    void _verify(bool boolExpression, const char* pstrCode, const char* pstrFunction, const char* pstrFileName, const int nLineNumber, const char* pstrMessage, ...);
} Kernel;

