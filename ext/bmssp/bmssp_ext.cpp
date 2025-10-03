// Ruby C extension for BMSSP
#include <ruby.h>
#include <cstdint>
#include <vector>
#include <stdexcept>

// Include BMSSP headers
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

// Ruby module and class handles
static VALUE rb_mBMSSP;
static VALUE rb_mBMSSPExt;

// Helper to convert Ruby array to C++ vector
static std::vector<uint64_t> rb_array_to_vector(VALUE rb_arr) {
  Check_Type(rb_arr, T_ARRAY);
  long len = RARRAY_LEN(rb_arr);
  std::vector<uint64_t> vec;
  vec.reserve(static_cast<size_t>(len));
  
  for (long i = 0; i < len; i++) {
    VALUE elem = rb_ary_entry(rb_arr, i);
    vec.push_back(static_cast<uint64_t>(NUM2ULL(elem)));
  }
  
  return vec;
}

// Helper to convert C++ vector to Ruby array
static VALUE vector_to_rb_array(const std::vector<uint64_t>& vec) {
  VALUE rb_arr = rb_ary_new_capa(static_cast<long>(vec.size()));
  
  for (size_t i = 0; i < vec.size(); i++) {
    rb_ary_push(rb_arr, ULL2NUM(vec[i]));
  }
  
  return rb_arr;
}

// Create a new graph
// @param n [Integer] number of vertices
// @return [Integer] pointer to Graph object (as Ruby Integer)
static VALUE bmssp_ext_create_graph(VALUE self, VALUE rb_n) {
  try {
    int n = NUM2INT(rb_n);
    if (n < 0) {
      rb_raise(rb_eArgError, "Number of vertices must be non-negative");
    }
    
    Graph* graph = new Graph(static_cast<size_t>(n));
    return ULL2NUM(reinterpret_cast<uintptr_t>(graph));
  } catch (const std::exception& e) {
    rb_raise(rb_eRuntimeError, "Failed to create graph: %s", e.what());
  } catch (...) {
    rb_raise(rb_eRuntimeError, "Unknown error creating graph");
  }
  
  return Qnil;
}

// Add an edge to the graph
// @param graph_ptr [Integer] pointer to Graph object
// @param u [Integer] source vertex
// @param v [Integer] destination vertex
// @param w [Integer] edge weight
// @return [NilClass]
static VALUE bmssp_ext_add_edge(VALUE self, VALUE rb_graph_ptr, VALUE rb_u, VALUE rb_v, VALUE rb_w) {
  try {
    uintptr_t ptr = NUM2ULL(rb_graph_ptr);
    Graph* graph = reinterpret_cast<Graph*>(ptr);
    
    int u = NUM2INT(rb_u);
    int v = NUM2INT(rb_v);
    uint64_t w = NUM2ULL(rb_w);
    
    graph->add_edge(u, v, w);
    
    return Qnil;
  } catch (const std::exception& e) {
    rb_raise(rb_eRuntimeError, "Failed to add edge: %s", e.what());
  } catch (...) {
    rb_raise(rb_eRuntimeError, "Unknown error adding edge");
  }
  
  return Qnil;
}

// Run SSSP from source vertex
// @param graph_ptr [Integer] pointer to Graph object
// @param source [Integer] source vertex
// @return [Array<Integer>] distances from source
static VALUE bmssp_ext_run_sssp(VALUE self, VALUE rb_graph_ptr, VALUE rb_source) {
  try {
    uintptr_t ptr = NUM2ULL(rb_graph_ptr);
    Graph* graph = reinterpret_cast<Graph*>(ptr);
    
    int source = NUM2INT(rb_source);
    
    std::vector<uint64_t> distances = run_sssp(*graph, source);
    
    return vector_to_rb_array(distances);
  } catch (const std::exception& e) {
    rb_raise(rb_eRuntimeError, "Failed to run SSSP: %s", e.what());
  } catch (...) {
    rb_raise(rb_eRuntimeError, "Unknown error running SSSP");
  }
  
  return Qnil;
}

// Free a graph object
// @param graph_ptr [Integer] pointer to Graph object
// @return [NilClass]
static VALUE bmssp_ext_free_graph(VALUE self, VALUE rb_graph_ptr) {
  try {
    uintptr_t ptr = NUM2ULL(rb_graph_ptr);
    if (ptr != 0) {
      Graph* graph = reinterpret_cast<Graph*>(ptr);
      delete graph;
    }
    
    return Qnil;
  } catch (const std::exception& e) {
    rb_raise(rb_eRuntimeError, "Failed to free graph: %s", e.what());
  } catch (...) {
    rb_raise(rb_eRuntimeError, "Unknown error freeing graph");
  }
  
  return Qnil;
}

// Initialize the extension
extern "C" void Init_bmssp_ext() {
  // Define BMSSP module
  rb_mBMSSP = rb_define_module("BMSSP");
  
  // Define BMSSP::Ext module for C extension functions
  rb_mBMSSPExt = rb_define_module_under(rb_mBMSSP, "Ext");
  
  // Define methods
  rb_define_module_function(rb_mBMSSPExt, "create_graph", 
                           reinterpret_cast<VALUE(*)(...)>(bmssp_ext_create_graph), 1);
  rb_define_module_function(rb_mBMSSPExt, "add_edge", 
                           reinterpret_cast<VALUE(*)(...)>(bmssp_ext_add_edge), 4);
  rb_define_module_function(rb_mBMSSPExt, "run_sssp", 
                           reinterpret_cast<VALUE(*)(...)>(bmssp_ext_run_sssp), 2);
  rb_define_module_function(rb_mBMSSPExt, "free_graph", 
                           reinterpret_cast<VALUE(*)(...)>(bmssp_ext_free_graph), 1);
}
