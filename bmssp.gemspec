# frozen_string_literal: true

require_relative "lib/bmssp/version"

Gem::Specification.new do |spec|
  spec.name = "bmssp"
  spec.version = BMSSP::VERSION
  spec.authors = ["George Baskervil"]
  spec.email = ["george@example.com"]

  spec.summary = "High-performance single-source shortest path algorithm"
  spec.description = <<~DESC
    BMSSP (Breaking the Sorting Barrier) implements a cutting-edge shortest path algorithm
    achieving O(m + n log^(2/3) n) time complexity for constant-degree graphs. This Ruby gem
    provides bindings to a highly optimized C++20 implementation with mimalloc, delivering
    13-23x speedup over conventional Dijkstra's algorithm.
  DESC
  spec.homepage = "https://github.com/georgebaskervil/duansalgorithm"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 2.7.0"

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = "https://github.com/georgebaskervil/duansalgorithm"
  spec.metadata["changelog_uri"] = "https://github.com/georgebaskervil/duansalgorithm/blob/main/CHANGELOG.md"

  # Specify which files should be added to the gem when it is released.
  spec.files = Dir[
    "lib/**/*.rb",
    "ext/**/*.{cpp,hpp,h}",
    "ext/**/extconf.rb",
    "README.md",
    "LICENSE",
    "CHANGELOG.md"
  ]
  
  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{\Aexe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions = ["ext/bmssp/extconf.rb"]

  # Development dependencies
  spec.add_development_dependency "rake", "~> 13.0"
  spec.add_development_dependency "rake-compiler", "~> 1.2"
  spec.add_development_dependency "minitest", "~> 5.0"
  spec.add_development_dependency "rubocop", "~> 1.0"
end
