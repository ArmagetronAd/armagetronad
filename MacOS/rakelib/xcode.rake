module AA::Xcode
  def self.process_file(orig, package_dir=nil)
    result_file = AA::Config.build_path(orig.ext)
    orig = AA::Config.src_path(orig)
  
    # Process the file (copy to build dir, replace tags)
    file result_file => [orig, directory(result_file.pathmap("%d"))] do |t|
      cp(orig, result_file)
    
      # replace the tags
      open(result_file, "r+") do |f|
        data = f.read
        AA::Config::TAG_MAPPINGS.each { |tag, value| data.gsub!("@#{tag}@", value) }
        f.rewind
        f.print(data)
        f.truncate(f.pos)
      end      
    end
    task "process-files" => result_file
  
    # Should the file be included with the game dist?
    if package_dir
      package_dest = AA::Config.package_path(package_dir, result_file.pathmap("%f"))
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
    if !File.exists?(AA::Config.build_path("resource"))
      if AA::Config::BUILD_TYPE == :development
        sh %{"#{AA::Config::SRC_DIR}/batch/make/sortresources" \\
             "#{AA::Config::SRC_DIR}/resource/proto" \\
             "#{AA::Config::BUILD_DIR}/resource/included" \\
             "#{AA::Config::SRC_DIR}/batch/make/sortresources.py"}
      else
        cp_r(AA::Config.src_path("resource"), AA::Config::BUILD_DIR)
      end
    end
  end
  
  task "package-resouces" do
    if !File.exists?(AA::Config.package_path("resource"))
      cp_r(AA::Config.build_path("resource"), AA::Config::PACKGAGE_RESOURCE_DIR)
    end
  end

  AA::Xcode.process_file("src/macosx/version.h.in")
  AA::Xcode.process_file("config/aiplayers.cfg.in", "config")  
  AA::Xcode.process_file("language/languages.txt.in", "language")
  if !AA::Config::DEDICATED
    AA::Xcode.process_file("src/macosx/English.lproj/InfoPlist.strings.in", "English.lproj")
  end
end
