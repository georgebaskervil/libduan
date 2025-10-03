# frozen_string_literal: true

require "mkmf"

# Set C++20 standard
$CXXFLAGS << " -std=c++20 -O3 -march=native"

# Check for mimalloc (optional but recommended)
if have_library("mimalloc")
  $CXXFLAGS << " -DHAVE_MIMALLOC"
  puts "Using mimalloc allocator"
end

# Add include directories
$INCFLAGS << " -I$(srcdir)/../../include"

# Specify source files explicitly
$srcs = ["bmssp_ext.cpp"]

# Add BMSSP implementation sources
bmssp_sources = %w[
  bmssp_algo.cpp
  bmssp_state.cpp
  bmssp.cpp
  find_pivots.cpp
  base_case.cpp
  structure.cpp
  instrumentation.cpp
  degree_check.cpp
]

# Copy source files to ext directory (for compilation)
bmssp_sources.each do |src|
  src_path = File.expand_path("../../src/#{src}", __dir__)
  if File.exist?(src_path)
    $srcs << src
    # Create rule to copy file
    FileUtils.cp(src_path, __dir__)
  else
    puts "Warning: Source file not found: #{src_path}"
  end
end

# Clean up any existing symlinks
Dir.glob("#{__dir__}/*.cpp").each do |f|
  File.delete(f) if File.symlink?(f)
end

create_makefile("bmssp/bmssp_ext")
