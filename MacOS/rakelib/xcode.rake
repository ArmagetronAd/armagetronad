module AAConfig
  def self.process_file(orig, package_dir=nil)
    result_file = build_path(orig.ext)
    orig = src_path(orig)
  
    # Process the file (copy to build dir, replace tags)
    file result_file => [orig, directory(result_file.pathmap("%d"))] do |t|
      cp(orig, result_file)
    
      # replace the tags
      open(result_file, "r+") do |f|
        data = f.read
        TAG_MAPPINGS.each { |tag, value| data.gsub!("@#{tag}@", value) }
        f.rewind
        f.print(data)
        f.truncate(f.pos)
      end      
    end
    task "process-files" => result_file
  
    # Should the file be included with the game dist?
    if package_dir
      package_dest = package_path(package_dir, result_file.pathmap("%f"))
      file package_dest => [result_file, directory(package_dest.pathmap("%d"))] do |t|
        cp(result_file, package_dest)
      end
      task "package-files" => package_dest
    end
  end
end

namespace "xcode" do
  
  task "prepare" => ["process-files", "sort-resources"]
  task "cleanup" => ["package-files", "package-resouces"]
  
  task "sort-resources" do
    if !File.exists?(AAConfig.build_path("resource"))
      if AAConfig::BUILD_TYPE == :development
        sh %{"#{AAConfig::SRC_DIR}/batch/make/sortresources" \\
             "#{AAConfig::SRC_DIR}/resource/proto" \\
             "#{AAConfig::BUILD_DIR}/resource/included" \\
             "#{AAConfig::SRC_DIR}/batch/make/sortresources.py"}
      else
        cp_r(AAConfig.src_path("resource"), AAConfig::BUILD_DIR)
      end
    end
  end
  
  task "package-resouces" do
    if !File.exists?(AAConfig.package_path("resource"))
      cp_r(AAConfig.build_path("resource"), AAConfig::PACKGAGE_RESOURCE_DIR)
    end
  end

  AAConfig.process_file("src/macosx/version.h.in")
  AAConfig.process_file("config/aiplayers.cfg.in", "config")  
  AAConfig.process_file("language/languages.txt.in", "language")
  if !AAConfig::DEDICATED
    AAConfig.process_file("src/macosx/English.lproj/InfoPlist.strings.in", "English.lproj")
  end
end
