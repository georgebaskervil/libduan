# frozen_string_literal: true

require "bundler/gem_tasks"
require "rake/testtask"
require "rake/extensiontask"

# Build the C extension
Rake::ExtensionTask.new("bmssp_ext") do |ext|
  ext.ext_dir = "ext/bmssp"
  ext.lib_dir = "lib/bmssp"
end

# Run tests
Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.libs << "lib"
  t.test_files = FileList["test/**/*_test.rb"]
end

# Default task: compile then test
task default: [:compile, :test]

# Clean task
task :clean do
  sh "rm -rf tmp pkg lib/bmssp/*.{so,bundle} ext/bmssp/*.o ext/bmssp/Makefile"
end
