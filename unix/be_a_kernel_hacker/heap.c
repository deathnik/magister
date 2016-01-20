// kernel device for sotoring 30 min strings
// baised on https://www.linuxvoice.com/be-a-kernel-hacker/
// many source code taken from https://github.com/vsinitsyn/reverse/blob/master/reverse.c menshioned in article above

#include <linux/init.h>		/* __init and __exit macroses */
#include <linux/kernel.h>	/* KERN_INFO macros */
#include <linux/module.h>	/* required for all kernel modules */
#include <linux/moduleparam.h>	/* module_param() and MODULE_PARM_DESC() */

#include <linux/fs.h>		/* struct file_operations, struct file */
#include <linux/miscdevice.h>	/* struct miscdevice and misc_[de]register() */
#include <linux/mutex.h>	/* mutexes */
#include <linux/string.h>	/* memchr() function */
#include <linux/slab.h>		/* kzalloc() function */
#include <linux/sched.h>	/* wait queues */
#include <linux/uaccess.h>	/* copy_{to,from}_user() */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("deathnik");
MODULE_DESCRIPTION("heap");

static unsigned long _element_size = 1024;
module_param(_element_size, ulong, (S_IRUSR | S_IRGRP | S_IROTH));
MODULE_PARM_DESC(_element_size, "Internal buffer size");

struct heap {
	struct mutex lock;
	unsigned int max_size;
	unsigned int cur_size;
	unsigned int element_size;
	char ** data;
} ;

#define PARENT(id) ((id-1)/2)
#define LEFT_CHILD(id) ((2 * id) + 1)
#define RIGHT_CHILD(id) ((2 * id) + 2)

//---- heap struct operations

void heap_pop(struct heap *h) {
    if (h->cur_size > 0) {
        char *tmp = h->data[0];
        h->data[0] = h->data[h->cur_size - 1];
        h->data[h->cur_size-1] = tmp;
        --h->cur_size;
    }
    // fix heap invariant
    if (h->cur_size > 0) {
        int id = 0;
        int lchild, rchild, min_ind;
        char *tmp;
        while (true) {
            lchild = LEFT_CHILD(id);
            rchild = RIGHT_CHILD(id);
            if (rchild >= h->cur_size) {
                if (lchild >= h->cur_size)
                    break;
                else
                    min_ind = lchild;
            } else {
                if (-strcmp(h->data[lchild], h->data[rchild]) <= 0)
                    min_ind = lchild;
                else
                    min_ind = rchild;
            }

            if (-strcmp(h->data[id], h->data[min_ind]) > 0) {
                tmp = h->data[min_ind];
                h->data[min_ind] = h->data[id];
                h->data[id] = tmp;
                id = min_ind;
            } else {
                break;
            }

        }
    }
}


//fix heap inv after pop
static void process_insert(struct heap *h) {
    int i = h->cur_size;
    int parent = PARENT(i);
    while (i > 0 && -strcmp(h->data[parent], h->data[i]) >= 0) {
        char *tmp = h->data[i];
        h->data[i] = h->data[parent];
        h->data[parent] = tmp;

        i = parent;
        parent = PARENT(i);
    }
    if (h->cur_size < h->max_size)
        ++h->cur_size;
}

static ssize_t print_heap(struct heap * h, char __user * out, size_t size, size_t readed){
	if ( h->cur_size == 0  || size == 0){
		return readed;
	}

	size_t copy_len = min(strlen(h->data[0]),size);
	if(copy_to_user(&out[readed],h->data[0],copy_len))
		return -EFAULT;
	heap_pop(h);
	return print_heap(h,out,size-copy_len,readed+copy_len);
}
//---- END of heap struct operations


//---- start of module functions
static struct heap * heap_alloc(unsigned long element_size){
	struct heap * h = kzalloc(sizeof(struct heap),GFP_KERNEL);
	if (unlikely(!h)){
		return h;
	}
	h->max_size = 30;
	h->element_size = _element_size;

	//allocate max_size +1 buffs for string storage
	// +1 will be used as temp
	h->data = kzalloc((h->max_size + 1) * sizeof(char*) , GFP_KERNEL);
	if (unlikely(!h->data)){
		kfree(h);
		return NULL;
	}
	for(int i=0; i< h->max_size +1; ++i){
		h->data[i] = kzalloc(_element_size *sizeof(char), GFP_KERNEL);
		if (unlikely(!h->data[i])){
			for(int j =0; j <i; ++j)
				kfree(h->data[j]);
			kfree(h->data);
			kfree(h);
			return NULL;
		}

	}

	mutex_init(&h->lock);
	h->cur_size = 0;

	return h;
}

static void heap_free(struct heap * h){
	for(int i=0; i< h->max_size +1; ++i)
		kfree(h->data[i]);
	kfree(h->data);
	kfree(h);
}


static int heap_open(struct inode *inode, struct file *file){
	struct heap *h;
	h = heap_alloc(_element_size);
	if (unlikely(!h)) {
		return -ENOMEM;
	}
	file->private_data = h;
	return 0;
}

static ssize_t heap_read(struct file *file, char __user * out,
			    size_t size, loff_t * off){
	printk(KERN_INFO"heap read call");
	struct heap * h = file->private_data;
	if (mutex_lock_interruptible(&h->lock))
		return -ERESTARTSYS;

	ssize_t result = print_heap(h,out,size,0);
	mutex_unlock(&h->lock);
	printk(KERN_INFO"heap read done ok");
	return result;
}

//called on heap_write
static ssize_t heap_write(struct file *file, const char __user * in,
			     size_t size, loff_t * off){
	//printk(KERN_INFO"heap write call\n");
	struct heap * h = file->private_data;
	if(size > _element_size)
		return -EFBIG;

	if (mutex_lock_interruptible(&h->lock))
		return -ERESTARTSYS;

    if(h->cur_size == h->max_size)
          heap_pop(h);

	if (copy_from_user(h->data[h->cur_size],in, size))
		return - EFAULT;

	process_insert(h);

	mutex_unlock(&h->lock);
	//printk(KERN_INFO"heap write done ok %i\n",h->cur_size);
	return size;
}

//free used mem
static int heap_close(struct inode *inode, struct file *file){
	struct heap *h = file->private_data;
	heap_free(h);
	return 0;
}

//standart template
static struct file_operations heap_fops = {
	.owner = THIS_MODULE,
	.open = heap_open,
	.read = heap_read,
	.write = heap_write,
	.release = heap_close,
	.llseek = noop_llseek
};

static struct miscdevice heap_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "heap",
	.fops = &heap_fops
};

static int __init heap_init(void){
	if (!_element_size) return -1;

	misc_register(&heap_misc_device);
	printk(KERN_INFO"heap device has been registered, msg size is %lu\n",_element_size);
	return 0;
}

static void __exit heap_exit(void){
	misc_deregister(&heap_misc_device);
	printk(KERN_INFO "heap device has been unregistered\n");
}

module_init(heap_init);
module_exit(heap_exit);
