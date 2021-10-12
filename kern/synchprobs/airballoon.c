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

/* Data structures for rope mappings */

/* Implement this! */
struct rope {
	struct lock *rope_lock;
	bool severed;
	int hook;
	int stake;
};

struct rope* rope_arr[NROPES];
int stake_arr[NROPES];
int hook_arr[NROPES];

struct rope *rope_create(const char *name, int num);

struct rope * rope_create (const char *name, int num) {
	struct rope *rope;

    rope = kmalloc(sizeof(struct rope));
    if (rope == NULL) {
            return NULL;
    }

	rope->rope_lock = lock_create(name);
	rope->severed = false;
	rope->hook = num;
	rope->stake = num;
	
	return rope;
}

/* Synchronization primitives */
struct lock* counter_lock;

/* Implement this! */

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
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
				kprintf("Dandelion severed rope %d\n", rope_num);
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
	kprintf("Dandelion thread done\n");
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
			//check to see if it is actually severed
			if (!rope_arr[rope_num]->severed) {
				//sever the rope
				rope_arr[rope_num]->severed = true;
				kprintf("Marigold severed rope %d from stake %d\n", rope_num, stake_num);
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
	kprintf("Marigold thread done\n");
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

	/* Implement this function */
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");
	while (ropes_left != 0);
	kprintf("Balloon freed and Prince Dandelion escapes\n");
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

	//mine
	(void)flowerkiller;
	(void)balloon;

	counter_lock = lock_create("counter");

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
/*
	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}
*/
	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
	return 0;
}
