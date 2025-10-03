#!/usr/bin/env ruby
# frozen_string_literal: true

require "bundler/setup"
require "bmssp"

# Example 1: Simple graph
puts "=== Example 1: Simple Graph ==="
graph = BMSSP::Graph.new(5)
graph.add_edge(0, 1, 10)
graph.add_edge(1, 2, 20)
graph.add_edge(0, 3, 5)
graph.add_edge(3, 2, 15)
graph.add_edge(3, 4, 25)

distances = graph.shortest_paths(0)
puts "Distances from vertex 0:"
distances.each_with_index do |dist, i|
  if dist == BMSSP::Graph::INF
    puts "  #{i}: INF (unreachable)"
  else
    puts "  #{i}: #{dist}"
  end
end

# Example 2: Using convenience method
puts "\n=== Example 2: Convenience Method ==="
edges = [
  [0, 1, 7],
  [0, 2, 9],
  [0, 5, 14],
  [1, 2, 10],
  [1, 3, 15],
  [2, 3, 11],
  [2, 5, 2],
  [3, 4, 6],
  [4, 5, 9]
]

distances = BMSSP.shortest_paths(6, edges, source: 0)
puts "Distances from vertex 0:"
distances.each_with_index do |dist, i|
  puts "  #{i}: #{dist}"
end

# Example 3: Large random graph
puts "\n=== Example 3: Large Random Graph ==="
require "benchmark"

n = 10_000
graph = BMSSP::Graph.new(n)

# Add random edges (constant degree ~2)
srand(42)
n.times do |u|
  degree = rand(1..3)
  degree.times do
    v = rand(n)
    next if v == u
    weight = rand(1..100)
    graph.add_edge(u, v, weight)
  end
end

time = Benchmark.realtime do
  distances = graph.shortest_paths(0)
  reachable = distances.count { |d| d < BMSSP::Graph::INF }
  puts "Graph: #{n} vertices, source 0"
  puts "Reachable vertices: #{reachable}"
end

puts "Time: #{(time * 1000).round(2)} ms"
puts "\nBMSSP achieves O(m + n log^(2/3) n) complexity!"
