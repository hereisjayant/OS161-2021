/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>


#define N_LORD_FLOWERKILLER 8
#define NROPES 16
static int ropes_left = NROPES;

//total threads = flowerkiller threads + dandelion, marigold, ballon and main
int threads_running = N_LORD_FLOWERKILLER + 4;

/* Data structures for rope mappings */

/* Implement this! */
struct rope {
	struct lock *rope_lock;
	bool severed;
	int num;
};

//arrays of ropes, stakes and hooks
struct rope* rope_arr[NROPES];
int stake_arr[NROPES];
int hook_arr[NROPES];

//functions for the ropes
struct rope *rope_create (const char *name, int num);
void rope_destroy (struct rope* rope);

struct rope * rope_create (const char *name, int num) {
	struct rope *rope;

    rope = kmalloc(sizeof(struct rope));
    if (rope == NULL) {
            return NULL;
    }

	rope->rope_lock = lock_create(name);
	rope->severed = false;
	rope->num = num;
	
	return rope;
}

void rope_destroy(struct rope* rope)
{
    KASSERT(rope != NULL);
	lock_destroy(rope->rope_lock);
    kfree(rope);
}

/* Synchronization primitives */
struct lock* counter_lock;
struct lock* cond_lock;
struct cv* cond_thread;

/* Implement this! */

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 * 
 * For dandelion, the thread will run while there is at least one rope left.
 * it first checks to see if a rope looks severed. If it does,
 * it continues and looks at another rope. If it looks unsevered, it will
 * obtain the lock and look again. If it has not been severed in the 
 * time it took to get the lock, it will sever the rope. it will then
 * release the lock. with a seperate lock, it will decrease the amount of
 * ropes left.
 * 
 * Marigold works much the same way, but instead of accessing ropes through
 * hook, it uses stakes. Additionally, it performs an additional check to 
 * see if the rope on a stake has changed since aquiring the lock, as
 * flowerkiller can switch it in between, which messes things up!
 * 
 * flowerkiller works like marigold, but it takes two ropes and switches them
 * instead of severing them. It is important that if two threads want the same lock,
 * they lock them in order, otherwise you would get a deadlock where thread A has
 * lock 1 and is waiting on lock 2, but thread B has lock 2 but is waiting on lock 1.
 * my implementation makes sure lock 1 will always be obtained before lock 2.
 * 
 * balloon doesnt have much to explain
 * 
 * the main waits on a condition variable.
 * 
 * all of these (except main) will exit after the amount of ropes is 0. just before it exits, it will
 * decrease the amount of threads running. Once the thread running count is 1, the
 * penultimate thread will signal main thread to wake, and so the main thread
 * will know all the other threads are done.
 * 
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Dandelion thread starting\n");

	while (ropes_left > 0) {
		//access rope through hook
		int hook_num = random() % NROPES;
		int rope_num = hook_arr[hook_num];
		//check if the rope looks severed
		if (!rope_arr[rope_num]->severed) {
			lock_acquire(rope_arr[rope_num]->rope_lock);
			//check to see if it is actually severed
			if (!rope_arr[rope_num]->severed) {
				//sever the rope
				rope_arr[rope_num]->severed = true;
				kprintf("Dandelion severed rope %d\n", rope_arr[rope_num]->num);
				lock_release(rope_arr[rope_num]->rope_lock);

				//decrease rope count
				lock_acquire(counter_lock);
				ropes_left--;
				lock_release(counter_lock);

				thread_yield();
			} else {
				lock_release(rope_arr[rope_num]->rope_lock);
			}
		}
	}
	lock_acquire(cond_lock);
	threads_running--;
	if (threads_running == 1) {
		cv_signal(cond_thread, cond_lock);
	}
	kprintf("Dandelion thread done\n");
	lock_release(cond_lock);

	
	thread_exit();

	/* Implement this function */
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Marigold thread starting\n");

	while (ropes_left > 0) {
		//access rope through stake
		int stake_num = random() % NROPES;
		int rope_num = stake_arr[stake_num];
		//check if the rope looks severed
		if (!rope_arr[rope_num]->severed) {
			lock_acquire(rope_arr[rope_num]->rope_lock);
			//check to see if rope_num changed since aquiring the lock
			if (rope_num != stake_arr[stake_num]) {
				lock_release(rope_arr[rope_num]->rope_lock);
				continue;
			}
			//check to see if it is actually severed
			if (!rope_arr[rope_num]->severed) {
				//sever the rope
				rope_arr[rope_num]->severed = true;
				kprintf("Marigold severed rope %d from stake %d\n", rope_arr[rope_num]->num, stake_num);
				lock_release(rope_arr[rope_num]->rope_lock);

				//decrease rope count
				lock_acquire(counter_lock);
				ropes_left--;
				lock_release(counter_lock);

				thread_yield();
			} else {
				lock_release(rope_arr[rope_num]->rope_lock);
			}
		}

	}
	lock_acquire(cond_lock);
	threads_running--;
	if (threads_running == 1) {
		cv_signal(cond_thread, cond_lock);
	}
	kprintf("Marigold thread done\n");
	lock_release(cond_lock);

	
	thread_exit();

	/* Implement this function */
}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Lord FlowerKiller thread starting\n");

	while (ropes_left > 0) {
		//access 2 ropes through stakes
		int stake1 = random() % NROPES;
		int stake2 = random() % NROPES;
		int rope1 = stake_arr[stake1];
		int rope2 = stake_arr[stake2];

		//if the ropes are the same, skip
		if (rope1 == rope2) {
			continue;
		}

		//make sure we know which rope is numerically smaller to distinguish ropes and avoid deadlocks
		struct rope* bigger_rope;
		struct rope* smaller_rope;
		int big_old, big_new, small_old, small_new;
		if (rope1 > rope2) {
			bigger_rope = rope_arr[rope1];
			big_old = stake1;
			big_new = stake2;
			smaller_rope = rope_arr[rope2];
			small_old = stake2;
			small_new = stake1;
		} else {
			bigger_rope = rope_arr[rope2];
			big_old = stake2;
			big_new = stake1;
			smaller_rope = rope_arr[rope1];
			small_old = stake1;
			small_new = stake2;
		}

		//check if the rope looks severed
		if (!bigger_rope->severed && !smaller_rope->severed) {
			//getting the locks in order means no deadlocks
			lock_acquire(bigger_rope->rope_lock);
			lock_acquire(smaller_rope->rope_lock);
			//check to see if ropes have been switched since last checking
			if (rope1 != stake_arr[stake1] || rope2 != stake_arr[stake2]) {
				lock_release(smaller_rope->rope_lock);
				lock_release(bigger_rope->rope_lock);
				continue;
			}
			//check to see if it is actually severed
			if (!bigger_rope->severed && !smaller_rope->severed) {
				//switch the ropes
				stake_arr[stake1] = rope2;
				stake_arr[stake2] = rope1;
				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", bigger_rope->num, big_old, big_new);
				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n", smaller_rope->num, small_old, small_new);
				lock_release(smaller_rope->rope_lock);
				lock_release(bigger_rope->rope_lock);

				thread_yield();
			} else {
				lock_release(smaller_rope->rope_lock);
				lock_release(bigger_rope->rope_lock);
			}
		}

	}
	lock_acquire(cond_lock);
	threads_running--;
	if (threads_running == 1) {
		cv_signal(cond_thread, cond_lock);
	}
	kprintf("Lord FlowerKiller thread done\n");
	lock_release(cond_lock);

	
	thread_exit();

	/* Implement this function */
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");
	while (ropes_left != 0) {
		thread_yield();
	}
	kprintf("Balloon freed and Prince Dandelion escapes\n");

	lock_acquire(cond_lock);
	threads_running--;
	if (threads_running == 1) {
		cv_signal(cond_thread, cond_lock);
	}
	kprintf("Balloon thread done\n");
	lock_release(cond_lock);

	
	thread_exit();

	/* Implement this function */
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0, i;

	(void)nargs;
	(void)args;
	(void)ropes_left;

	counter_lock = lock_create("counter");
	cond_lock = lock_create("cond");
	cond_thread = cv_create("cv");

	//fill in arrays
	for (i = 0; i < NROPES; i++) {
		stake_arr[i] = i;
		hook_arr[i] = i;
		rope_arr[i] = rope_create("rope", i);
	}

	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	while (threads_running != 1) {
		lock_acquire(cond_lock);
		cv_wait(cond_thread, cond_lock);
		lock_release(cond_lock);
	}

	for (i = 0; i < NROPES; i++) {
		rope_destroy(rope_arr[i]);		
	}

	lock_destroy(counter_lock);
	lock_destroy(cond_lock);
	cv_destroy(cond_thread);
	kprintf("Main thread done\n");

	//set it up so we can do sp1 again
	ropes_left = NROPES;
	threads_running = N_LORD_FLOWERKILLER + 4;
	return 0;
}
