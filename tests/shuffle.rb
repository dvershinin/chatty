#! /usr/bin/env ruby

# 
# The algorithm is extracted from:
# <http://www.rubygarden.org/ruby?OneLiners>
#
class Array
  def shuffle
    each_index {|j|
      i = rand(size-j);
      self[j], self[j+i] = self[j+i], self[j]
    }
  end
end
puts readlines.shuffle
