# For this to work correctly, you must have an entry like the following in your ~/.ssh/config:
# 
#   Host aabeta
#       HostName beta.armagetronad.net
#       User ...
# 

module AA::AABeta
  
  def self.version_branch
    File.read(AA::Config.src_path("major_version")).chomp
  end
  
  RELEASES_LIST_DIR = AA::Config.generated_path("www-aabeta")
  RELEASES_LIST = AA::Config.generated_path("www-aabeta/releases.php")
  
  DMG_NAME = AA::Release.dmg_name + ".dmg"
  DMG_FILE = AA::Release.dmg_path + ".dmg"
  BETA_DIR_SHORT = "#{version_branch()}/#{AA::Config.version}"
  BETA_DIR = "/var/www/armagetron/distfiles/#{BETA_DIR_SHORT}"
  
  SORT_BY = %w[file source branch date tag os arch format packager server]
  
  def self.upload(file, beta_dir)
    beta_file = AA::Config.combine_path_components(beta_dir, file.pathmap("%f"))
    
    sh("ssh aabeta mkdir -m 2775 -p #{beta_dir}")
    sh("scp #{AA::Config.escape_sh file} aabeta:#{beta_dir}")
    sh("ssh aabeta chmod 664 #{beta_file}")
  end
  
  def self.add_to_releases_list(params)
    assoc = make_php_assoc(params)
    
    releases = File.readlines(RELEASES_LIST)
    insert_at = releases.index("/* === INSERT NEW RELEASES AFTER THIS LINE === */\n")
    
    releases.insert(insert_at + 1, assoc)
    
    File.open(RELEASES_LIST, "w") do |f|
      f.write(releases.join)
    end
  end
    
  # Returns a php assoc suitable to be put into releases.php
  def self.make_php_assoc(params)
    # Make the release in the same order as others
    params = params.to_a.sort_by { |(key, _)| SORT_BY.index(key) }
    php_params = params.map { |(k, v)| "    #{k.inspect} => #{v.inspect}" }

    "array(\n" + php_params.join(",\n") + "\n),\n"
  end
  
  
  def self.checkout_releases_list
    sh("svn co -N \\
          https://armagetronad.svn.sourceforge.net/svnroot/armagetronad/www/beta/trunk/www-aabeta \\
          #{AA::Config.escape_sh RELEASES_LIST_DIR}")
  end
  
  def self.update_releases_list
    sh("svn up #{AA::Config.escape_sh RELEASES_LIST}")
  end
  
  def self.commit_releases_list(file_name)
    msg = AA::Config.escape_sh("Added #{file_name}")
    sh("svn ci -m #{msg} #{AA::Config.escape_sh RELEASES_LIST}")
  end
  
  def self.arch
    archs = (ENV["ARCHS"] || "i386").split
    if archs.length > 1
      "ppc_32 & x86_32"
    else
      archs[0]
    end
  end
  
  def self.packager
    ENV["AABETA_PACKAGER"] || "dlh"
  end
  
end

namespace "aabeta" do
  
  task "checkout" do
    if !File.exist?(AA::AABeta::RELEASES_LIST_DIR)
      AA::AABeta.checkout_releases_list
    end
  end
  
  task "update" do
    AA::AABeta.update_releases_list
  end
  
  task "upload" do
    AA::AABeta.upload(AA::AABeta::DMG_FILE, AA::AABeta::BETA_DIR)
  end
  
  task "add-to-releases" do
    dmg = AA::Release.dmg_name + ".dmg"
    
    params = {
      "file" => "#{AA::AABeta::BETA_DIR_SHORT}/#{AA::AABeta::DMG_NAME}",
      "branch" => AA::AABeta.version_branch,
      "date" => Time.now.strftime("%Y-%m-%d"),
      "os" => "Mac OS X 10.3+",
      "arch" => AA::AABeta.arch,
      "format" => ".dmg",
      "packager" => AA::AABeta.packager,
    }
    
    if AA::Config::DEDICATED
      params["server"] = true
    end
    
    AA::AABeta.add_to_releases_list(params)
  end
  
  task "commit-releases" do
    AA::AABeta.commit_releases_list(AA::AABeta::DMG_NAME)
  end
  
  task("release" => ["checkout", "update", "upload", "add-to-releases", "commit-releases"])
end
