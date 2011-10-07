# Setup load-path
$LOAD_PATH.unshift(File.expand_path("./"))

# Make directory return a useful value
alias :original_directory :directory
def directory(dir)
  original_directory(dir)
  Rake::Task[dir]
end

require "rakelib/config"

module AA
  CONFIG = AA::Config.new
end

AA::CONFIG.version_info.initialize_info

import "rakelib/xcode.rake"
import "rakelib/release.rake"
import "rakelib/aabeta.rake"

task "remove-version" do
  rm_rf(AA::CONFIG.generated_path("src", "macosx"))
  if AA::CONFIG.build_type == :development
    rm_f(AA::CONFIG::version_info.version_file)
  end
end

desc "Update version"
task "update-version" => ["remove-version", "xcode:process-files"]

