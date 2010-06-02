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

# required gems/modules
require 'open3'
require 'open4'
require 'timeout'
require 'spec/expectations'

#################### "Before" and "After" steps ####################

After do
    # make sure that if @pid is set, that process has been terminated
    if @pid
        Process.kill("KILL", @pid)
    end
end


#################### "Given" steps ####################

#
# Set up the basic feed handler name
#
Given /^an (\w+) feed handler$/ do |fh|
    @fh = fh
end

#
# If the test is expecting compilation this step is necessary to demonstrate where the FH code is
#
Given /^with code in (.+)$/ do |compile_dir|
    @compile_dir = File.join(ROOT_DIR, compile_dir)
end

#
# If the test is expecting to run a feed handler this step is necessary to determine the name of
# the binary to run
#
Given /^with a binary named (\w+)$/ do |binary|
    @binary = binary
end

#
# If the test will need to run a feed handler with certain arguments, this step will allow them to
# be set
#
Given /^with the arguments "(.+)"$/ do |args|
    @args = args
end


#################### "When" steps ####################

#
# This step will compile a feed handler, as long as @compile_dir has been set
#
When /^the feed handler is compiled$/ do
    # throw an exception if @compile_dir has not been set
    throw Exception.new("the instance variable @compile_dir must be set") unless @compile_dir

    Open3.popen3("make -C #{@compile_dir} dist") do |stdin, stdout, stderr|
        while output = stderr.gets
            throw Exception.new("compilation failed in #{@compile_dir}") if output =~ /\*\*\*/
        end
    end
end

#
# This step will run a feed handler, as long as the necessary instance variables have been set
#
When /^run$/ do
    # throw an exception if @fh or @binary has not been set
    throw Exception.new("the instance variable @fh must be set") unless @fh
    throw Exception.new("the instance variable @binary must be set") unless @binary

    # set up some directories and comamnds that will be used to run the requested feed handler
    @dist_dir  = File.join(ROOT_DIR, Dir.entries(ROOT_DIR).select{|entry| entry =~ /^dist_/}.first)
    @command   = "#{@dist_dir}/fh/#{@fh.downcase}/bin/#{@binary.downcase}"
    @command  += " #{@args}" if @args

    # run the feed handler setting @pid, @stdin, @stdout, and @stderr instance variables
    ENV['FH_HOME'] = "#{@dist_dir}/fh"
    @pid, @stdin, @stdout, @stderr = Open4::popen4("#{@command}")
end


#################### "Then" steps ####################

#
# This step ensures that the feed handler produces a certain string in a certain period of time
#
Then /^it should produce the string "(.+)" within (\d+) seconds$/ do |string, seconds|
    # throw an exception if @stdout has not been set
    throw Exception.new("the instance variable @stdout must be set") unless @stdout

    # set up a timeout for the specified number of seconds while we look for the specified string
    success = false
    begin
        status = Timeout.timeout(seconds.to_i) do
            while output = @stdout.gets
                if output =~ /#{string}/
                    success = true
                    break
                end
            end
        end
    # if we get a Timeout::Error then we have exceeded the specified timeout and thus throw
    # a new, more descriptive exception (and not an error) so that Cucumber can react
    rescue(Timeout::Error)
        throw Exception.new("timed out while waiting #{seconds} seconds for string '#{string}'")
    end

    # if success local variable is not true (string was found), throw an exception
    throw Exception.new("feed handler exited before string '#{string}' was found") unless success
end

#
# This step ensures that the feed handler terminates when sent a certain signal within a certain
# period of time.  It also saves the exit code for later steps
#
Then /^terminate within (\d+) seconds$/ do |seconds|
    # throw an exception if @pid has not been set
    throw Exception.new("the instance variable @pid must be set") unless @pid

    # set up a timeout for the specified number of seconds while we look for the specified string
    begin
        status = Timeout.timeout(seconds.to_i) {
            pid, status = Process.wait2(@pid)
            @exitcode   = status.exitstatus
            @pid        = nil
        }
    # if we get a Timeout::Error then we have exceeded the specified timeout and thus throw
    # a new, more descriptive exception (and not an error) so that Cucumber can react
    rescue(Timeout::Error)
        throw Exception.new("timed out while waiting #{seconds} seconds for process '#{@pid}'")
    end
end

#
# This step ensures that the feed handler terminates when sent a certain signal within a certain
# period of time.  It also saves the exit code for later steps
#
Then /^terminate on the signal (\w+) within (\d+) seconds$/ do |signal, seconds|
    # throw an exception if @pid has not been set
    throw Exception.new("the instance variable @pid must be set") unless @pid

    # send the process @pid the specified signal
    Process.kill(signal, @pid)

    # set up a timeout for the specified number of seconds while we look for the specified string
    begin
        status = Timeout.timeout(seconds.to_i) {
            pid, status = Process.wait2(@pid)
            @exitcode   = status.exitstatus
            @pid        = nil
        }
    # if we get a Timeout::Error then we have exceeded the specified timeout and thus throw
    # a new, more descriptive exception (and not an error) so that Cucumber can react
    rescue(Timeout::Error)
        throw Exception.new("timed out while waiting #{seconds} seconds for process '#{@pid}'")
    end
end

#
# This step checks to make sure that a previously terminated process exited with the correct
# exit status code
#
Then /^exit with code (\d+)$/ do |code|
    # throw an exception if @exitcode has not been set
    throw Exception.new("the instance variable @exitcode must be set") unless @exitcode

    @exitcode.should eql(code.to_i)
end

#
# This step checks to make sure that a previously terminated process did not exit with an incorrect
# exit status code
#
Then /^not exit with code (\d+)$/ do |code|
    # throw an exception if @exitcode has not been set
    throw Exception.new("the instance variable @exitcode must be set") unless @exitcode

    @exitcode.should_not eql(code.to_i)
end
