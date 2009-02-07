require "rakelib/config"

class AA::Xcode
  
  def initialize
    @generated_resource_dir = AA::CONFIG.generated_path("resource")

    @tag_mappings = {
      "version" => AA::CONFIG.version,
      "year" => Time.now.strftime("%Y"),
      "progtitle" => AA::CONFIG.product_name,
    }.merge(AA::CONFIG::version_info.info)
  end
  
  attr_reader :generated_resource_dir
  attr_reader :tag_mappings
  
  def process_file(orig, package_dir=nil)
    result_file = AA::CONFIG.generated_path(orig.ext)
    orig = AA::CONFIG.top_path(orig)
  
    # Process the file (copy to build dir, replace tags)
    file result_file => [orig, directory(result_file.pathmap("%d"))] do |t|
      cp(orig, result_file)
    
      # replace the tags
      open(result_file, "r+") do |f|
        data = f.read
        @tag_mappings.each { |tag, value| data.gsub!("@#{tag}@", value) }
        f.rewind
        f.print(data)
        f.truncate(f.pos)
      end      
    end
    task "process-files" => result_file
  
    # Should the file be included with the game dist?
    if package_dir
      package_dest = AA::CONFIG.package_path(package_dir, result_file.pathmap("%f"))
      file package_dest => [result_file, directory(package_dest.pathmap("%d"))] do |t|
        cp(result_file, package_dest)
      end
      task "package-files" => package_dest
    end
  end
  
  def sort_resources
    resource_included = AA::CONFIG.combine_path_components(@generated_resource_dir, "included")
    
    if AA::CONFIG.build_type == :development
      sh %{"#{AA::CONFIG.top_dir}/batch/make/sortresources" \\
           "#{AA::CONFIG.top_dir}/resource/proto" \\
           #{AA::Config.escape_sh resource_included} \\
           "#{AA::CONFIG.top_dir}/batch/make/sortresources.py"}
    else
      cp_r(AA::CONFIG.top_path("resource"), AA::CONFIG.generated_path)
    end
  end
  
end

AA::XCODE = AA::Xcode.new

namespace "xcode" do
  task "prepare" => ["process-files", "sort-resources"]
  task "cleanup" => ["package-files", "package-resouces"]
  
  task "sort-resources" do
    if !File.exists?(AA::XCODE.generated_resource_dir)
      AA::XCODE.sort_resources
    end
  end
  
  task "package-resouces" do
    if !File.exists?(AA::CONFIG.package_path("resource"))
      cp_r(AA::XCODE.generated_resource_dir, AA::CONFIG.packgage_resource_dir)
    end
  end

  task "process-files"
  AA::XCODE.process_file("src/macosx/version.h.in")
  AA::XCODE.process_file("config/aiplayers.cfg.in", "config")  
  AA::XCODE.process_file("language/languages.txt.in", "language")
  if !AA::CONFIG.dedicated?
    AA::XCODE.process_file("src/macosx/English.lproj/InfoPlist.strings.in", "English.lproj")
  end
end
