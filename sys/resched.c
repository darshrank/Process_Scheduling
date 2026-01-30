/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>
#include <math.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */

int expdist_sched() {
	register struct pentry *optr; /* pointer to old process entry */
	register struct pentry *nptr; /* pointer to new process entry */
	int candidate;
	double randval = expdev(0.1); // Example parameter for the exponential distribution
	
	optr = &proctab[currpid];

	if(optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid, rdyhead, optr->pprio);
	}

	candidate = q[rdyhead].qnext;

	while(candidate != rdytail && proctab[candidate].pprio <= randval) {
		candidate = q[candidate].qnext;
	}

	if(candidate == rdytail) {
		// If no candidate found, pick the highest priority process
		candidate = getlast(rdytail);
	} 
	else {
		// Remove the chosen candidate from the ready list
		dequeue(candidate);
	}

	// if (candidate == currpid) {
	// 	// If the chosen candidate is the current process, just return
	// 	optr->pstate = PRCURR;
	// #ifdef RTCLOCK
	// 	preempt = QUANTUM;
	// #endif

	// 	kprintf(" %d\n", currpid);
	// 	return OK;
	// }

	currpid = candidate;
	nptr = &proctab[currpid];
	nptr->pstate = PRCURR; /* mark it currently running */
	#ifdef RTCLOCK
		preempt = QUANTUM; /* reset preemption counter */
	#endif
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	// kprintf(" %d\n", currpid);
	return OK;
}; 

int linux_sched() {
    register struct pentry *optr;  
    register struct pentry *nptr;  
    int next;                      
    int i;

    optr = &proctab[currpid];

    if (optr->pstate == PRCURR) {
        optr->counter = preempt;
        if (optr->counter <= 0) {
            optr->goodness = 0;
        } else {
            optr->goodness = optr->pprio + optr->counter;
        }

        optr->pstate = PRREADY;
        insert(currpid, rdyhead, optr->goodness);
    }

    int max_goodness = 0;
    next = NULLPROC;

    for (i = 0; i < NPROC; i++) {
        if (proctab[i].pstate == PRREADY && proctab[i].goodness > max_goodness) {
            max_goodness = proctab[i].goodness;
            next = i;
        }
    }

    if (max_goodness == 0) {
		// kprintf("linux_sched: starting new epoch\n");
        for (i = 0; i < NPROC; i++) {

            if (proctab[i].pstate != PRFREE && i != NULLPROC) {
                if (proctab[i].counter <= 0 || proctab[i].quantum == 0) {
                    proctab[i].quantum = proctab[i].pprio;
                } else {
                    proctab[i].quantum = proctab[i].pprio + (proctab[i].counter / 2);
                }
                proctab[i].counter = proctab[i].quantum;
                proctab[i].goodness = proctab[i].pprio + proctab[i].counter;
            }
        }

        max_goodness = 0;
        next = NULLPROC;
        for (i = 0; i < NPROC; i++) {
            if (proctab[i].pstate == PRREADY && proctab[i].goodness > max_goodness) {
                max_goodness = proctab[i].goodness;
                next = i;
            }
        }

        
    }
	if (next == NULLPROC) {
		nptr = &proctab[NULLPROC];
		nptr->pstate = PRCURR;
		currpid = NULLPROC;
#ifdef RTCLOCK
		preempt = QUANTUM;
#endif
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		return OK;
	}

    if(next != NULLPROC)
		dequeue(next);
    nptr = &proctab[next];
    nptr->pstate = PRCURR;
    currpid = next;

#ifdef RTCLOCK
    preempt = nptr->counter;
	// preempt = 23;
#endif

    ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

    return OK;
}


int default_sched() {
	
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}
	
	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);	

	return OK;
};

int resched()
{	
	switch( sched_class ) {
	    case EXPDISTSCHED:
	        expdist_sched();
	        break;
	    case LINUXSCHED:
	        linux_sched();
	        break;
	    default:
			default_sched();
			break;
	}
	return OK;
}
