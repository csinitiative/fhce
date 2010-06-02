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

# set up the root path paths, relative to this file
ROOT_DIR = File.dirname(__FILE__) + '/../../..'

# set up the distribution directory, relative to the root
DIST_DIR = File.join(ROOT_DIR, 'dist_' + `make -s -C #{ROOT_DIR} buildstring`.chomp, 'fh')

#
# Clean up after anything steps in this file might have done
#
After do
    if @original_dir
        FileUtils.cd(@original_dir)
        @original_dir = nil
    end
end

#
# Change directory (before test begins)
#
Given /^directory is changed to "(.+)"$/ do |dir|
    @original_dir = FileUtils.pwd
    if dir =~ /^\//
        FileUtils.cd(dir)
    else
        FileUtils.cd(File.join(ROOT_DIR, dir))
    end
end
