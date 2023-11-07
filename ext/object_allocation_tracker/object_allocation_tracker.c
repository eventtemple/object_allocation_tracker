#include <ruby/ruby.h>
#include <ruby/debug.h>

static size_t allocation_count = 0;
static VALUE tracepoint = Qnil;

/*
  Event hook that fires when an object is allocated. This is registered with the tracepoint, and increments the
  allocation count.

  :nodoc:
*/
static void event_hook(VALUE tpval, void *data)
{
  allocation_count++;
}

/*
  Registers the tracepoint that will fire when an object is allocated, and sets the allocation count to 0.

  :nodoc:
*/
static VALUE start_allocation_count(VALUE self)
{
  allocation_count = 0;

  /*
    Enable the tracepoint only for object allocation events. Need to do some testing to see what the performance
    implications are of registering the tracepoints are at the block level. If it's too much overhead then we'll
    need to register the tracepoint when the extension is loaded, and figure out a way to ignore events unless the
    call has actually happened, and maintain a list of allocation counts by caller that get freed.
  */
  tracepoint = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, event_hook, (void *)NULL);

  rb_tracepoint_enable(tracepoint);

  return Qnil;
}

/*
  Stops the tracepoint that was registered from firing, after the block has finished executing.

  :nodoc:
*/
static VALUE stop_allocation_count(VALUE self)
{
  if (!NIL_P(tracepoint))
  {
    rb_tracepoint_disable(tracepoint);
  }

  return Qnil;
}

/* Document-method: start
 *
 * call-seq:
 *   ObjectAllocationTracker.start -> 1000
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
static VALUE count_allocations_within_block(VALUE self)
{
  start_allocation_count(self);
  rb_ensure(rb_yield, Qundef, stop_allocation_count, self);

  return SIZET2NUM(allocation_count);
}

/*
  Initialization function for the extension. This is called when the extension is loaded into the VM, and creates a
  new class called ObjectAllocationTracker, and a class method of start that takes a block and returns the number of
  object allocations that occurred within the block.

  :TODO: Further testing should be done to ensure this is truely thread-safe. I believe it is, but there could be some
  edge cases, like if threads aren't properly syncronized by calling libraries for whatever reason.

  :TODO: It may make sense to provide a mechanism in the tracking to record child allocations by an identifier of the caller,
  and allow the reset of the stored aggregate count. This would allow the called to track allocations by specific groupings
  like a call to a specific framework, class, etc. Something to think about for the future.

  :nodoc:
*/
void Init_object_allocation_tracker(void)
{
  VALUE cObjectAllocationTracker = rb_define_class("ObjectAllocationTracker", rb_cObject);
  rb_define_singleton_method(cObjectAllocationTracker, "start", count_allocations_within_block, 0);
}
