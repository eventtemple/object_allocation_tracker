# frozen_string_literal: true
#
def track_thread_allocations(objects_count)
  thread = Thread.new do
    begin
      allocations, result = ObjectAllocationTracker.start do
        objects_count.times { Object.new }
      end

      { thread: Thread.current, allocations: allocations, result: result }
    rescue => e
      puts "Exception in thread: #{e.class}: #{e.message}"
      e.backtrace.each { |line| puts "\t#{line}" }
      raise
    end
  end.value
end

RSpec.describe ObjectAllocationTracker do
  it "has a version number" do
    expect(ObjectAllocationTracker::VERSION).not_to be nil
  end

  describe ".count" do
    it "tracks object allocations within block" do
        # TODO: An extra allocation is being assigned here when running the test suite if an initial
        # trace isn't being done first. It may be due to the intialization of the tracepoint or tls
        # instance, but need to investigate further. For now, just doing an initial trace to avoid
        # the extra allocation.
        track_thread_allocations(7)

        results = track_thread_allocations(5)

        expect(results[:allocations]).to eq(5)
    end

    it "does not track object allocations outside block" do
      track_thread_allocations(1000)

      results = track_thread_allocations(50)
      expect(results[:allocations]).to eq(50)
    end

    it "tracks object allocations properly" do
      objects_to_allocate = Array.new(500) { rand(1...5000) }
      object_allocation_results = objects_to_allocate.collect do |object_count|
         track_thread_allocations(object_count)
      end

      threads = object_allocation_results.collect { |result| result[:thread] }
      allocations = object_allocation_results.collect { |result| result[:allocations] }

      threads.each(&:join)

      object_allocation_results.each_with_index do |result, index|
        expect(result[:allocations]).to eq(objects_to_allocate[index])
      end
    end

    it 'raises proper exception when error is raised within a called block' do
      example_block = Proc.new { raise 'error' }
      expect { ObjectAllocationTracker.start(&example_block) }.to raise_error(RuntimeError, 'error')
    end

  end
end
