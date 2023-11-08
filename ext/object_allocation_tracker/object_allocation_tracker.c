#include <ruby/ruby.h>
#include <ruby/debug.h>

static ID id_allocation_count;

/*
  Event hook that fires when an object is allocated. This is used to track the allocations and
  utilizes thread local storage to keep track of the count per thread, and should be thread safe.

  :nodoc:
*/
static void event_hook(VALUE tpval, void *data)
{
  VALUE current_count_value = rb_thread_local_aref(rb_thread_current(), id_allocation_count);
  size_t current_count = 0;

  if (!NIL_P(current_count_value))
  {
    current_count = NUM2SIZET(current_count_value);
  }

  current_count++;

  rb_thread_local_aset(rb_thread_current(), id_allocation_count, SIZET2NUM(current_count));
}

/*
  Registers the tracepoint that will fire when an object is allocated, and sets the starting count on the thread's
  local storage to the default 0.

  :nodoc:
*/
static VALUE start_allocation_count(VALUE self)
{
  rb_thread_local_aset(rb_thread_current(), id_allocation_count, SIZET2NUM(0));
  VALUE tracepoint = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, event_hook, NULL);
  rb_tracepoint_enable(tracepoint);

  // Debug output
  fprintf(stderr, "Starting allocation count for thread: %ld\n", rb_thread_id(rb_thread_current()));

  return tracepoint;
}

/*
  Stops the tracepoint that was registered from firing any further, and sets the threads locally stored count to
  nil so that it can be garbage collected.

  :nodoc:
*/
static VALUE stop_allocation_count(VALUE tp)
{
  rb_tracepoint_disable(tp);
  VALUE count = rb_thread_local_aref(rb_thread_current(), id_allocation_count);

  fprintf(stderr, "Stopping allocation count for thread: %ld\n", rb_thread_id(rb_thread_current()));
  fprintf(stderr, "Allocation count is: %lu\n", NUM2SIZET(count));

  rb_thread_local_aset(rb_thread_current(), id_allocation_count, Qnil);
  return count;
}

/* Document-method: start
 *
 * call-seq:
 *   ObjectAllocationTracker.start {} -> 1000
 *
 * Begins tracking object allocations.
 *
 * This method enables the tracking which will count objects allocated during a block execution. The block will return
 * the number of objects allocated during the block execution.
 *
 *   ObjectAllocationTracker.start do
 *     # ... code that creates objects ...
 *   end
 */
static VALUE rb_object_allocation_tracker_start(VALUE self)
{
  VALUE tp = start_allocation_count(self);

  return rb_ensure(rb_yield, Qundef, stop_allocation_count, tp);
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
  rb_define_singleton_method(cObjectAllocationTracker, "start", rb_object_allocation_tracker_start, 0);
  id_allocation_count = rb_intern("allocation_count");
}
