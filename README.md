# Object Allocation Tracker

This project provides a simple, alternative, and performant way to track object allocations without impacting
performance as much object space tracing does from within Ruby. It's useful for gathering metrics in production-like
environments for debugging memory bloat and leak issues.

This stemmed from needing better allocation data within New Relic, and takes a similar approach to how both Scout and
AppSignal gather data for their APM systems:

- [AppSignal Implementation](https://github.com/appsignal/appsignal-ruby/blob/main/ext/appsignal_extension.c#L818)
- [Scout Implementation](https://github.com/scoutapp/scout_apm_ruby/blob/master/ext/allocations/allocations.c)

A few other Ruby libraries like Stackprof also utilize the approach to generate metrics:

- [Stackprof](https://github.com/tmm1/stackprof/blob/master/ext/stackprof/stackprof.c#L199)

## Installation

Install the gem and add to the application's Gemfile by executing:

    $ bundle add object_allocation_tracker

If bundler is not being used to manage dependencies, install the gem by executing:

    $ gem install object_allocation_tracker

## Usage
```ruby
      allocations = ObjectAllocationTracker.start do
        100.times { |_| Object.new }
      end
```

## Development

After checking out the repo, run `bin/setup` to install dependencies. Then, run `rake spec` to run the tests. You can also run `bin/console` for an interactive prompt that will allow you to experiment.

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/eventtemple/object_allocation_tracker.
