# frozen_string_literal: true

require "test_helper"

class BMSSPTest < Minitest::Test
  def test_version
    refute_nil ::BMSSP::VERSION
  end

  def test_create_graph
    graph = BMSSP::Graph.new(5)
    assert_equal 5, graph.size
  end

  def test_add_edge
    graph = BMSSP::Graph.new(3)
    assert_nil graph.add_edge(0, 1, 10)
    assert_nil graph.add_edge(1, 2, 20)
  end

  def test_shortest_paths_simple
    graph = BMSSP::Graph.new(4)
    graph.add_edge(0, 1, 10)
    graph.add_edge(1, 2, 20)
    graph.add_edge(0, 3, 5)
    graph.add_edge(3, 2, 15)

    distances = graph.shortest_paths(0)
    
    assert_equal 4, distances.length
    assert_equal 0, distances[0]
    assert_equal 10, distances[1]
    assert_equal 20, distances[2]
    assert_equal 5, distances[3]
  end

  def test_shortest_paths_unreachable
    graph = BMSSP::Graph.new(5)
    graph.add_edge(0, 1, 10)
    graph.add_edge(1, 2, 20)
    # Vertices 3 and 4 are unreachable

    distances = graph.shortest_paths(0)
    
    assert_equal BMSSP::Graph::INF, distances[3]
    assert_equal BMSSP::Graph::INF, distances[4]
  end

  def test_convenience_method
    edges = [
      [0, 1, 10],
      [1, 2, 20],
      [0, 3, 5],
      [3, 2, 15]
    ]

    distances = BMSSP.shortest_paths(4, edges, source: 0)
    
    assert_equal 4, distances.length
    assert_equal 0, distances[0]
    assert_equal 10, distances[1]
    assert_equal 20, distances[2]
    assert_equal 5, distances[3]
  end

  def test_invalid_vertex
    graph = BMSSP::Graph.new(3)
    
    assert_raises(ArgumentError) { graph.add_edge(-1, 1, 10) }
    assert_raises(ArgumentError) { graph.add_edge(0, 5, 10) }
    assert_raises(ArgumentError) { graph.shortest_paths(-1) }
    assert_raises(ArgumentError) { graph.shortest_paths(5) }
  end

  def test_invalid_weight
    graph = BMSSP::Graph.new(3)
    
    assert_raises(ArgumentError) { graph.add_edge(0, 1, -5) }
  end

  def test_large_graph
    n = 1000
    graph = BMSSP::Graph.new(n)
    
    # Create a chain
    (n - 1).times do |i|
      graph.add_edge(i, i + 1, 1)
    end

    distances = graph.shortest_paths(0)
    
    assert_equal 0, distances[0]
    assert_equal 1, distances[1]
    assert_equal n - 1, distances[n - 1]
  end
end
