# frozen_string_literal: true

RSpec.describe ObjectAllocationTracker do
  it "has a version number" do
    expect(ObjectAllocationTracker::VERSION).not_to be nil
  end

  describe ".count" do
    it "tracks object allocations within block" do
      allocations = ObjectAllocationTracker.start do
        100.times { |_| Object.new }
      end

      expect(allocations).to be(101)
    end

    it "does not track object allocations outside block" do
      1000.times { |_| Object.new }

      allocations = ObjectAllocationTracker.start do
        50.times { |_| Object.new }
      end

      expect(allocations).to be(51)
    end

    it "tracks object allocations within multiple threads properly" do
      thread = Thread.new do
        ObjectAllocationTracker.start do
          100.times { |_obj| Object.new }
        end
      end

      thread2 = Thread.new do
        ObjectAllocationTracker.start do
          20.times { |_obj| Object.new }
        end
      end

      [thread, thread2].each { |t| t.join }

      expect(thread.value).to eq(101)
      expect(thread2.value).to eq(21)
    end
  end
end
