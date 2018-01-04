#ifndef LINUX_RCUPDATE_H
#define LINUX_RCUPDATE_H

struct rcu_head {
};

#define __rcu
#define rcu_dereference_protected(p, c)	(p)
#define RCU_INIT_POINTER(p, v)		do { (p) = (v); } while(0)
#define rcu_read_lock()
#define rcu_read_unlock()

#endif
