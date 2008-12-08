# Make directory return a useful value
alias :original_directory :directory
def directory(dir)
  original_directory(dir)
  Rake::Task[dir]
end

module AAConfig
  def self.combine_path_components(base, *components)
    File.join(*([base] + components))
  end
  
  def self.src_path(*components)
    combine_path_components(SRC_DIR, *components)
  end
  
  def self.build_path(*components)
    combine_path_components(BUILD_DIR, *components)
  end
  
  def self.package_path(*components)
    combine_path_components(PACKGAGE_RESOURCE_DIR, *components)
  end
  
  def self.version
    if BUILD_TYPE == :development
      %x("#{SRC_DIR}/batch/make/version" "#{SRC_DIR}").chomp
    else
      File.read("#{SRC_DIR}/src/macosx/version.h.in").scan(/#define VERSION "(.*)"/)[0][0]
    end
  end
    
  # Paths are relative the Rakefile
  SRC_DIR = File.expand_path(File.dirname(__FILE__) + "/..")
  
  BUILD_DIR = ENV["SYMROOT"] || "build"
  
  PRODUCT_NAME = ENV["PRODUCT_NAME"] || "Armagetron Advanced"
  
  DEDICATED = !!PRODUCT_NAME[/dedicated/i]
  
  PACKGAGE_RESOURCE_DIR = [
    ENV["CONFIGURATION_BUILD_DIR"] || (BUILD_DIR + "/Debug"),
    DEDICATED ? "" : PRODUCT_NAME + (ENV["WRAPPER_SUFFIX"] || "") + "/Contents/Resources"
  ].join("/")
    
  BUILD_TYPE = File.exists?(src_path(".svn")) ? :development : :release
    
  TAG_MAPPINGS = {
    "version" => version(),
    "year" => Time.now.strftime("%Y"),
    "progtitle" => PRODUCT_NAME,
  }
  
  PROGRAM_SHORT_NAME = DEDICATED ? "armagetronad" : "armagetronad-dedicated"
end

task "remove-version" do
  rm_rf(build_path("src", "macosx"))
end

desc "Update version"
task "update-version" => ["remove-version", "xcode:process-files"]

