# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'rbhelium/version'
require 'rake'

Gem::Specification.new do |spec|
  spec.name          = "rbhelium"
  spec.version       = Helium::VERSION
  spec.authors       = ["Patrick Thomson"]
  spec.email         = ["patrick@helium.co"]
  spec.summary       = "Ruby interface to the Helium platform."
  spec.homepage      = ""
  spec.license       = "MIT"

  spec.files         = `git ls-files -z`.split("\x0")
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]
  spec.extensions    = Rake::FileList["ext/**/extconf.rb"]

  spec.add_development_dependency "bundler", "~> 1.7"
  spec.add_development_dependency "rake", "~> 10.0"
end

