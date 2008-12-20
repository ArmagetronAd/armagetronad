module AA::Release
  def self.create_dmg(directory, volume_name)
    dmg = temp_file()
    sh("hdiutil create -srcdir #{AA::Config.escape_sh directory} \\
          -format UDIF #{AA::Config.escape_sh dmg} \\
          -volname #{AA::Config.escape_sh volume_name}")
    
    dmg + ".dmg"
  end
  
  def self.compress_dmg(uncompressed, new_compressed_name)
    sh("hdiutil convert #{AA::Config.escape_sh uncompressed} \\
          -format UDZO \\
          -o #{AA::Config.escape_sh new_compressed_name}")
    nil
  end
  
  def self.dmg_name
    AA::Config.configuration_build_path("#{AA::Config::PROGRAM_SHORT_NAME}-#{AA::Config.version}.macosx-#{AA::Config.arch}")
  end
  
  def self.temp_file
    %x{mktemp -t armagetronad}.chomp
  end
  
  def self.temp_directory
    %x{mktemp -dt armagetronad}.chomp
  end
end

namespace "release" do
  
  task "dmg" do
    release_directory = AA::Release.temp_directory
    final_dmg = AA::Release.dmg_name
    
    final_dmg_plus_ext = final_dmg + ".dmg"
    if File.exists?(final_dmg_plus_ext)
      rm(final_dmg_plus_ext)
    end
    
    # Gather files to package
    cp_r(AA::Config::PACKGAGE_RESOURCE_DIR_BASE, release_directory)
    
    # Make the dmg
    uncompressed_dmg = AA::Release.create_dmg(release_directory, AA::Config::PRODUCT_NAME)
    AA::Release.compress_dmg(uncompressed_dmg, final_dmg)
    
    # Cleanup
    rm(uncompressed_dmg)
    rm_rf(release_directory)
  end
  
end
