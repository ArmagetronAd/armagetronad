# Make directory return a useful value
alias :original_directory :directory
def directory(dir)
  original_directory(dir)
  Rake::Task[dir]
end

module AA
end

import "rakelib/config.rake"
import "rakelib/xcode.rake"
import "rakelib/release.rake"
import "rakelib/aabeta.rake"

task "remove-version" do
  rm_rf(AA::Config.generated_path("src", "macosx"))
end

desc "Update version"
task "update-version" => ["remove-version", "xcode:process-files"]

