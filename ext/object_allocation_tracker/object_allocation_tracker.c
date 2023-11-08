#include <ruby.h>
#include <ruby/debug.h>

static ID id_allocation_count;

/*
  Event hook that fires when an object is allocated. This is used to track the allocations and
  utilizes thread local storage to keep track of the count per thread, and should be thread safe.

  :nodoc:
*/
static void event_hook(VALUE tpval, void *data)
{
  VALUE thread = rb_thread_current();
  VALUE current_count_value = rb_thread_local_aref(thread, id_allocation_count);
  size_t current_count = (NIL_P(current_count_value)) ? 0 : NUM2SIZET(current_count_value);

  current_count++;

  rb_thread_local_aset(thread, id_allocation_count, SIZET2NUM(current_count));
}

/* Document-method: start
 *
 * call-seq:
 *   ObjectAllocationTracker.start {} -> 1000
 *
 * Begins tracking object allocations.
 *
 * This method enables the tracking which will count objects allocated during a block execution. The block will return
 * the number of objects allocated during the block execution, and the results of the block executed
 *
 *   allocations, result = ObjectAllocationTracker.start do
 *     # ... code that creates objects ...
 *   end
 */
static VALUE start(VALUE self)
{
  rb_thread_local_aset(rb_thread_current(), id_allocation_count, SIZET2NUM(0));

  VALUE tp = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, event_hook, NULL);
  rb_tracepoint_enable(tp);

  int state;
  VALUE result = rb_protect(rb_yield, Qundef, &state);

  rb_tracepoint_disable(tp);

  VALUE count = rb_thread_local_aref(rb_thread_current(), id_allocation_count);
  rb_thread_local_aset(rb_thread_current(), id_allocation_count, Qnil);

  if (state)
  {
    rb_jump_tag(state);
  }

  return rb_ary_new_from_args(2, count, result);
}

/*
  Initialization function for the extension. This is called when the extension is loaded into the VM, and creates a
  new class called ObjectAllocationTracker, and a class method of start that takes a block and returns the number of
  object allocations that occurred within the block.

  :nodoc:
*/
void Init_object_allocation_tracker(void)
{
  VALUE cObjectAllocationTracker = rb_define_class("ObjectAllocationTracker", rb_cObject);
  id_allocation_count = rb_intern("allocation_count");
  rb_define_singleton_method(cObjectAllocationTracker, "start", start, 0);
}
