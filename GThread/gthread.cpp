/*---------------------------------------
 * gthread - poweful user thread made simple
 *
 * Programmer: Gustavo Campos
 * 
 * Date : 22-08-2017
 *
 * IMPLEMENTATION
 
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
#define INT_CEILING(x,y) ((uint64_t)((1LL + ((x - 1LL) / y))*y))
#define INT_LOWEST(x,y) (x<y ? x : y)
#define INT_GREATEST(x,y) (x>y ? x : y)

#define verify(x, y, z...) _verify(x,#x, __PRETTY_FUNCTION__, __FILE__, __LINE__, y,  z)

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

    srand((uint)getTimeInMicroSec());
}




inline uint64_t as_microseconds(struct timespec* ts) {
    return ts->tv_sec * (uint64_t)1000000L + ts->tv_nsec / 1000;
}

uint64_t gthread::getTimeInMicroSec (void)
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return (as_microseconds(&spec));
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
	
    getcontext(&thContext->uContext);

    thContext->uContext.uc_stack.ss_sp = mmap(0, nStack, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE| MAP_ANON, -1, 0);
    
	thContext->uContext.uc_stack.ss_size = nStack;
    thContext->uContext.uc_stack.ss_flags = 0;
    thContext->uContext.uc_link = &threadKernel.uContext;

    makecontext (&thContext->uContext, pFunction, 0);

    thContext->nThreadStatus  = GTHREAD_STATUS_INIT;
    thContext->pstrThreadName = pstrName;
    thContext->nPriority      = nPriority;

	errno=0;

    verify(errno == 0, "ERROR, [%s] creating tread.\n", strerror(errno));

    //printf ("[%s] Created thread [%s] nice: [%u] Status: [%u ticks]\n", __PRETTY_FUNCTION__, pstrName, nPriority, thContext->nThreadStatus);

    vecThread.push_back (thContext);

    /* re-order */
    sortPriorityDescendently(vecThread);
}





const char* gthread::getCurrentThreadName()
{
    return pThreadWorking->pstrThreadName;
}




#define GTHREAD_USLEEP2TICK(x) (x/GTHREAD_TICK_USEC)

uint64_t gthread::markNextRunningContexts(uint64_t nTick)
{
    //struct { uint64_t nValue; thread* pThread;} stCandidate = { 0, NULL };

    uint64_t nLowest=-1;
    uint64_t nCurrentTIme = 0;
    uint64_t nTimeFactor=0;

    vector<thread*>::iterator it;

    for (it = vecThread.begin(); it != vecThread.end(); ++it)
    {
        nCurrentTIme = getTimeInMicroSec();
        
        nTimeFactor = (((*it)->nSleepETA > 0 && (*it)->nSleepETA > nCurrentTIme) ? (((uint64_t)(*it)->nSleepETA - nCurrentTIme) / GTHREAD_TICK_USEC) + 1 : 0) + (*it)->nPriority;
        
        (*it)->nNextRunningTick = INT_CEILING(nTick, nTimeFactor );

        nLowest = INT_LOWEST((*it)->nNextRunningTick, nLowest);
    }

    //printf ("Kernel: The Closest tick is: [%llu] sleep (%lld)\n", nLowest, (nLowest - nTick));

    return nLowest;
}




void gthread::Start ()
{
    uint64_t nRunningETA;
    vector<thread*>::iterator it;
    uint64_t nCiclo=1;
    uint64_t nTime;

    while (true || (++nTick))
    {
        if (nTick > ((uint64_t) 0xFFFFFFFFFFFF + GTHREAD_TICK_USEC)) { nTick= nTick - UINT16_MAX; nCiclo++; continue; }

        usleep ((useconds_t)((nRunningETA=markNextRunningContexts(nTick)) - nTick) * GTHREAD_TICK_USEC);

        nTick += (nRunningETA -nTick);
        
        for (it = vecThread.begin(); it != vecThread.end(); ++it)
        {
            if ((*it)->nNextRunningTick == nRunningETA)
            {
                
                 nTime = getTimeInMicroSec();
                
                //printf ("\n\tKernel:   nTick: [%u] Next: [%u] diff: [%u] -- Time: [%llu] -SLeep ETA: [%llu] diff: [%llu] \n", nTick, nTick + (nRunningETA -nTick), nRunningETA - nTick, nTime, (*it)->nSleepETA, (*it)->nSleepETA - nTime);
                
                
                if ((*it)->nThreadStatus == GTHREAD_STATUS_SLEEP && (*it)->nSleepETA <= nTime)
                {
                    (*it)->nThreadStatus = GTHREAD_STATUS_RUNNING;
                    (*it)->nSleepETA = 0;
                    
                    //printf ("\n\t ELIPSED TIME: [%f] \n", (double) (nTime - (*it)->nStartTime) / 1000000);
                }
                
                if ( (((*it)->nThreadStatus == GTHREAD_STATUS_RUNNING || (*it)->nThreadStatus == GTHREAD_STATUS_INIT)) )
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





bool gthread::Microleep(uint64_t nuTime)
{
    verify(pThreadWorking != NULL, "Logic Error, it should have priviously been initilizaed. (%llu)\n", NULL);

    //printf ("\t->[%s] executing sleep.", pThreadWorking->pstrThreadName);
    
    pThreadWorking->nThreadStatus = GTHREAD_STATUS_SLEEP;
    pThreadWorking->nSleepETA = getTimeInMicroSec() + nuTime;
    pThreadWorking->nStartTime = getTimeInMicroSec();
    

    swapcontext(&pThreadWorking->uContext, &threadKernel.uContext);

    
    return true;
}




bool gthread::Continue()
{
    verify(pThreadWorking != NULL, "Logic Error, it should have priviously been initilizaed. (%llu)\n", Kernel.getCurrentTick());
    
    printf ("\t->[%s] executing Switching.\n\n", pThreadWorking->pstrThreadName);
    
    swapcontext(&pThreadWorking->uContext, &threadKernel.uContext);
    
    return true;
}



const uint64_t gthread::getCurrentTick()
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
        Kernel.Microleep(500000LL);
	}
}



void Function2 ()
{
    size_t nValue = 10; //(size_t) ppszValues[0];
    
    const char* pstrThreadName = Kernel.getCurrentThreadName ();
    
    while (Kernel.Continue())
    {
        printf ("%s - Value: [%lu] Tick: [%u]\n", pstrThreadName, nValue++, Kernel.getCurrentTick());
    }
}


int main (int nArgs, char** ppszArgs)
{
    Kernel.CreateThread ((void (*)())Function, 100, "Thread 1", 0, NULL);

    Kernel.CreateThread ((void (*)())Function, 1000, "Thread 2", 0, (void**)20);

    Kernel.CreateThread ((void (*)())Function2, 100, "Thread 3", 0, NULL);
    
    Kernel.Start();

	return 0;
}

