#  This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
# 
#  CSI FH is free software: you can redistribute it and/or modify it under the terms of the
#  GNU Lesser General Public License as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#  
#  CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public License
#  along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.

# required gems/modules
require 'open4'
require 'spec/expectations'

#
# Support function to determine if a particular string exists in stderr or stdout
#
def string_in_stream?(string, stream)
    # set the output variable to the stream that was specified (making sure the proper array
    # of output has been set)
    if stream == 'stdout'
        throw Exception.new("the instance variable @output must be set") unless @output
        output = @output
    else
        throw Exception.new("the instance variable @errors must be set") unless @errors
        output = @errors
    end
    
    # check each line of the output for the specified string and return if the string is found
    output.each do |line|
        if line =~ /#{string}/
            return true
        end
    end
    
    false
end

#
# Run the specified command
#
When /^the command "(.+)" is run$/ do |command|
    `#{command}`
end

#
# Run the specified command with captured output
#
When /^the command "(.+)" is run with captured output$/ do |command|
    # capture each of stderr and stdout while running the specified command
    @output = @errors = []

    @status = Open4.popen4("#{command}") do |pid, stdin, stdout, stderr|
        while output = stdout.gets
            @output << output.chomp
        end
        while error = stderr.gets
            @errors << error.chomp
        end
    end
    
    @exitcode = @status.exitstatus
end

#
# Set up the binary that is going to be run
#
When /^the binary (.+) is used$/ do |binary|
    @binary = binary
end

#
# Set up the binary that is going to be run (in the dist directory)
#
When /^the dist binary (.+) is used$/ do |binary|
    dist_dir = 'dist_' + `make -s -C #{ROOT_DIR} buildstring`.chomp
    @binary  = File.join(ROOT_DIR, dist_dir, 'fh', binary)
end

#
# Set the arguments variable to the specified string
#
When /^passed arguments "(.+)"$/ do |args|
    @args = args
end

#
# Set the specified environment variable
#
When /^(\w+) is set to "(.+)"$/ do |variable, value|
    # if the value given is a "special" value (certain word in []) set it to the interpreted value
    case value
    when "[DIST]"
        value = DIST_DIR
    end
    
    ENV[variable] = value
end


#
# Run the specified binary and capture stderr and stdout
#
When /^run with captured output$/ do
    # throw an exception if @binary
    throw Exception.new("the instance variable @binary must be set") unless @binary

    # capture each of stderr and stdout while running the specified binary (with arguments)
    # if specified
    @output = []
    @errors = []
    @pid = Open3.popen3("#{@binary} #{@args if @args}") do |x, stdout, stderr|
        while output = stdout.gets
            @output << output.chomp
        end
        while error = stderr.gets
            @errors << error.chomp
        end
    end
end

#
# Check the specified stream for the specified string
#
Then /^"(.+)" is seen on (stdout|stderr)$/ do |string, stream|
    unless string_in_stream?(string, stream)
        throw Exception.new("string '#{string}' not found in the #{stream} stream")
    end
end

#
# Check the specified stream for the absence of a specified string
#
Then /^"(.+)" is not seen on (stdout|stderr)$/ do |string, stream|
    if string_in_stream?(string, stream)
        throw Exception.new("string '#{string}' found in the #{stream} stream")
    end
end
