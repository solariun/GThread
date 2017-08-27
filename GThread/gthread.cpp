/*---------------------------------------
 * gthread - poweful user thread made simple
 *
 * Programmer: Gustavo Campos
 * 
 * Date : 22-08-2017
 *
 * IMPLEMENTATION
 
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

#include "gthread.h"
#include <stdio.h>


#define KERNEL_CONTEXT 0;

#define GTHREAD_STATUS_KERNEL	255

#define GTHREAD_STATUS_NILL 	0 
#define GTHREAD_STATUS_INIT		1
#define GTHREAD_STATUS_RUNNING	2
#define GTHREAD_STATUS_SLEEP	3
#define GTHREAD_STATUS_HALT		4
#define GTHREAD_STATUS_STOPPED	5


#define GTHREAD_TICK_USEC   500


/* 64bits length, be AWARE OF IT */
#define INT_CEILING(x,y) ((uint64_t)1 + ((x - 1) / y))*y
#define INT_LOWEST(x,y) (x<y ? x : y)
#define INT_GREATEST(x,y) (x>y ? x : y)

#define verify(x, y, z...) _verify(x,#x, __PRETTY_FUNCTION__, __FILE__, __LINE__, z)

static unsigned int nDefaultStackSize = 163841;


void gthread::_verify (bool boolExpression, const char* pstrCode, const char* pstrFunction, const char* pstrFileName, const int nLineNumber, const char* pstrMessage, ...)
{
    if (boolExpression == false)
    {
        va_list vaList;
        va_start(vaList, pstrMessage);

        fprintf (stderr, "Error @%s:%u (%s) : [%s] ", pstrFunction, nLineNumber, pstrFileName, pstrCode);
        vfprintf(stderr, pstrMessage, vaList);
        write(fileno(stderr), "\n", 1);

        va_end(vaList);
        
        exit(1);
    }
}

void gthread::sortPriorityDescendently(vector<thread*> &vec)
{
    /* sort with LAMBDA C++ expression */
    sort (vec.begin(), vec.end(), [](thread* thFirst, thread* thSecond){ return INT_GREATEST(thFirst->nPriority, thSecond->nPriority);});
}



gthread::gthread()
{
	printf ("Inicializando nano Kernel\n");

    srand((uint)getTimeInNanoSec());
}



uint64_t gthread::getTimeInNanoSec (void)
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

	return (uint64_t)((uint64_t) spec.tv_sec + (uint64_t) spec.tv_nsec);
}




void gthread::CreateThread(void (*pFunction)(), uint16_t nPriority, const char* pstrName, uint nArgs, void** ppArgs)
{
	CreateThread(pFunction, nDefaultStackSize, nPriority, pstrName, nArgs, ppArgs);
}



void gthread::firstCallFunction (void (*pFunction)(), thread *pThread)
{

}

void gthread::CreateThread(void (*pFunction)(), uint64_t nStack, uint16_t nPriority, const char* pstrName, uint nArgs, void** ppArgs)
{
	thread *thContext = new thread();
	
	uint8_t *ui8StackeData = new uint8_t (nStack);


    getcontext(&thContext->uContext);

	thContext->uContext.uc_stack.ss_sp = ui8StackeData;
	thContext->uContext.uc_stack.ss_size = nStack;
    thContext->uContext.uc_stack.ss_flags = 0;
    thContext->uContext.uc_link = &threadKernel.uContext;

    makecontext (&thContext->uContext, pFunction, 0);

    thContext->nThreadStatus  = GTHREAD_STATUS_INIT;
    thContext->pstrThreadName = pstrName;
    thContext->nPriority      = nPriority;

	errno=0;


	if (errno != 0)
	{
		fprintf (stderr, "ERROR, [%s] creating tread.\n", strerror(errno));
	}

    //printf ("[%s] Created thread [%s] nice: [%u] Status: [%u ticks]\n", __PRETTY_FUNCTION__, pstrName, nPriority, thContext->nThreadStatus);

    vecThread.push_back (thContext);

    /* re-order */
    sortPriorityDescendently(vecThread);
}



const char* gthread::getCurrentThreadName()
{
    return pThreadWorking->pstrThreadName;
}



useconds_t gthread::markNextRunningContexts(useconds_t nTick)
{
    //struct { uint64_t nValue; thread* pThread;} stCandidate = { 0, NULL };

    useconds_t nLowest=-1;

    vector<thread*>::iterator it;

    for (it = vecThread.begin(); it != vecThread.end(); ++it)
    {
        (*it)->nNextRunningTick = INT_CEILING(nTick, (*it)->nPriority);

        //printf ("\tKernel: [%20s] Nice: [%4u]  Status: [%3u] NextBeta:[%u] Tick: [%u] SleepTime: [%u]\n", (*it)->pstrThreadName, (*it)->nPriority, (*it)->nThreadStatus, (*it)->nNextRunningTick, nTick, (*it)->nNextRunningTick - nTick);

        nLowest = INT_LOWEST((*it)->nNextRunningTick, nLowest);
    }

    //printf ("Kernel: The Closest tick is: [%u] sleep (%u)\n", nLowest, (nLowest - nTick));

    return nLowest;
}



void gthread::Start ()
{
    useconds_t nRunningETA;
    vector<thread*>::iterator it;
    uint64_t nCiclo=1;

    while (true || (++nTick))
    {
        if (nTick > (UINT16_MAX + GTHREAD_TICK_USEC)) { nTick= nTick - UINT16_MAX; nCiclo++; continue; }

        usleep ((useconds_t)((nRunningETA=markNextRunningContexts(nTick)) - nTick) * GTHREAD_TICK_USEC);

        //printf ("\n\tKernel:   nTick: [%u] Next: [%u] diff: [%u]\n", nTick, nTick + (nRunningETA -nTick), nRunningETA -nTick);

        nTick += (nRunningETA -nTick);

        for (it = vecThread.begin(); it != vecThread.end(); ++it)
        {
            if ((*it)->nNextRunningTick == nRunningETA)
            {
                
                if ((*it)->nThreadStatus == GTHREAD_STATUS_SLEEP)
                {
                    
                }

                if (((*it)->nThreadStatus == GTHREAD_STATUS_RUNNING || (*it)->nThreadStatus == GTHREAD_STATUS_INIT))
                {
                    pThreadWorking = (*it);

                    //printf ("Kernel: Executing [%s] Nice: [%u] RUNNING Tick: [%llu : %u] \n", pThreadWorking->pstrThreadName, pThreadWorking->nPriority, nCiclo, nTick);

                    swapcontext(&threadKernel.uContext, &pThreadWorking->uContext);


                    if (pThreadWorking->nThreadStatus == GTHREAD_STATUS_INIT) pThreadWorking->nThreadStatus = GTHREAD_STATUS_RUNNING;

                    nTick++;
                }
            }
        }

    }

    //swapcontext(&threadKernel->uContext, &(*it)->uContext);

}





bool gthread::USleep(useconds_t nuTime)
{
    verify(pThreadWorking == NULL, "Logic Error, it should have priviously been initilizaed. (%llu)\n", NULL);

    printf ("\t->[%s] executing sleep.", pThreadWorking->pstrThreadName);
    
    pThreadWorking->nThreadStatus = GTHREAD_STATUS_SLEEP;
    pThreadWorking->nSleepTime = nuTime;
    pThreadWorking->nStartTime = getTimeInNanoSec();
    

    swapcontext(&pThreadWorking->uContext, &threadKernel.uContext);

    
    return true;
}


bool gthread::Continue()
{
    verify(pThreadWorking == NULL, "Logic Error, it should have priviously been initilizaed. (%llu)\n", NULL);
    
    printf ("\t->[%s] executing Switching.", pThreadWorking->pstrThreadName);
    
    swapcontext(&pThreadWorking->uContext, &threadKernel.uContext);
    
    return true;
}



const useconds_t gthread::getCurrentTick()
{
    return nTick;
}



/*
 *  MAIN E Funcitons for testing
 */

void Function ()
{
    size_t nValue = 10; //(size_t) ppszValues[0];
    
    const char* pstrThreadName = Kernel.getCurrentThreadName ();
    
	while (Kernel.Continue())
	{
        printf ("%s - Value: [%lu] Tick: [%u]\n", pstrThreadName, nValue++, Kernel.getCurrentTick());
        usleep (1000);
	}
}



int main (int nArgs, char** ppszArgs)
{
    Kernel.CreateThread ((void (*)())Function, 100, "Thread 1", 0, (void**)20);

    Kernel.CreateThread ((void (*)())Function, 1000, "Thread 2", 0, (void**)20);

    Kernel.Start();

	return 0;
}

