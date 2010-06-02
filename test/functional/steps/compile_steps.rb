#  Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
#
#  This file is part of FeedHandlers (FH).
#
#  FH is free software: you can redistribute it and/or modify it under the terms of the
#  GNU Lesser General Public License as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#
#  FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with FH.  If not, see <http://www.gnu.org/licenses/>.

require 'spec/expectations'

#
# Run a make on a certain directory with a certain target
#
When /^a make is performed on (.+) with target (.+)$/ do |dir, target|
    @compile_dir = "#{ROOT_DIR}/#{dir}"
    Open3.popen3("make -C #{@compile_dir} #{target unless target == '[NONE]'}") do |x, y, stderr|
        while output = stderr.gets
            throw Exception.new("compilation failed in #{@compile_dir}") if output =~ /\*\*\*/
        end
    end
end

#
# Make sure that the specified file exists
#
Then /^the file (\w+) should be produced$/ do |binary|
    # throw an exception if @compile_dir has not been set
    throw Exception.new("the instance variable @compile_dir must be set") unless @compile_dir

    bin_dir = Dir.entries(@compile_dir).select{|entry| entry =~ /^bin_/}.first
    File.exists?("#{@compile_dir}/#{bin_dir}/#{binary}").should == true
end

#
# Make sure that no build products exist within the @compile_dir
#
Then /^there should be no build products left$/ do
    # throw an exception if @compile_dir or @fh has not been set
    throw Exception.new("the instance variable @compile_dir must be set") unless @compile_dir
    throw Exception.new("the instance variable @fh must be set") unless @fh

    build_string = `make -s -C #{ROOT_DIR} buildstring`.chomp
    Dir.glob("#{@compile_dir}/**/*").each do |filename|
        filename.match(/#{build_string}/).should == nil
        filename.match(/fh_#{@fh.downcase}_revision\.h/).should == nil
    end
end

#
# Check for a standard distribution file layout for a feed handler in the dist directory
#
Then /^a standard directory structure should exist in the dist directory$/ do
    # throw an exception if @fh has not been set
    throw Exception.new("the instance variable @fh must be set") unless @fh

    # make sure all the right directories are there
    build_string = `make -s -C #{ROOT_DIR} buildstring`.chomp
    dist_dir = File.join(ROOT_DIR, 'dist_' + build_string, 'fh', @fh.downcase)
    File.exists?(dist_dir).should == true
    File.directory?(dist_dir).should == true
    [ 'bin', 'etc', 'plugins' ].each do |dir|
        real_dir = File.join(dist_dir, dir)
        File.exists?(real_dir).should == true
        File.directory?(real_dir).should == true
    end

    # make sure all the right files are there
    [ 'bin/fhitch_v1', 'etc/itch.conf' ].each do |file|
        real_file = File.join(dist_dir, file)
        File.exists?(real_file).should == true
        File.directory?(real_file).should == false
    end

end