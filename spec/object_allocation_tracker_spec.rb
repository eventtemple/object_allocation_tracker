# frozen_string_literal: true

def track_thread_allocations(objects_count)
  Thread.new do
    begin
      ObjectAllocationTracker.start do
        objects_count.times { Object.new }
      end
    rescue => e
      puts "Exception in thread: #{e.class}: #{e.message}"
      e.backtrace.each { |line| puts "\t#{line}" }
      raise
    end
  end
end

RSpec.describe ObjectAllocationTracker do
  it "has a version number" do
    expect(ObjectAllocationTracker::VERSION).not_to be nil
  end

  describe ".count" do
    it "tracks object allocations within block" do
      allocations = ObjectAllocationTracker.start do
        100.times { |_| Object.new }
      end

      expect(allocations).to be(100)
    end

    it "does not track object allocations outside block" do
      1000.times { |_| Object.new }

      allocations = ObjectAllocationTracker.start do
        50.times { |_| Object.new }
      end

      expect(allocations).to be(50)
    end

    it "tracks object allocations within multiple threads properly" do
      object_allocation_counts = Array.new(1000) { rand(1...5000) }
      object_allocation_threads = object_allocation_counts.collect { |objects_count| track_thread_allocations(objects_count) }

      object_allocation_threads.each(&:join)

      object_allocation_threads.each_with_index do |thread, index|
        expect(thread.value).to eq(object_allocation_counts[index])
      end
    end

    it 'raises proper exception when error is raised within a called block' do
      example_block = Proc.new { raise 'error' }
      expect { ObjectAllocationTracker.start(&example_block) }.to raise_error(RuntimeError, 'error')
    end

  end
end
