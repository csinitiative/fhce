#!/usr/bin/ruby

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

# use Open3 so we can capture stderr
require 'open3'
require 'getoptlong'
include Open3

# define some class constants for console colors
CONSOLE_COLOR_RED    = "[0;31m"
CONSOLE_COLOR_GREEN  = "[0;32m"
CONSOLE_COLOR_YELLOW = "[0;33m"
CONSOLE_RESET        = "[0m"


# function to recursively collect tests, starting at '..'
def collect_tests(directory)
    tests = []
    
    # get a list of directory entries in the current directory that will be considered
    entries = Dir.entries(directory).reject do |entry|
        entry =~ /^\./ || entry == 'www'
    end
    
    # collect tests for the directories that have not been rejected
    entries.each do |file|
        realfile = "#{directory}/#{file}"
        if File.directory?(realfile)
            tests += collect_tests(realfile)
        elsif file =~ /\.test$/ && File.executable?(realfile)
            tests.push(realfile)
        end
    end
    
    tests
end


# function to process a test summary string
def process_test(summary, message)
    test, file, line, result = summary.split(/:/)
    
    case result
    when 'SUCCESS'
        print '.'
        
    when 'FAILURE'
        print 'F'
        @messages << "Failure in #{test} (#{file}:#{line})\n#{message}"
        
    else
        print 'E'
        @messages << "Error in #{test} (#{file}:#{line})\n#{message}"
        
    end
end

# function to process an end of test file summary line
def process_summary(summary)
    discard, tests, assertions, failures, errors = summary.split(/:/)
    
    @tests      += tests.to_i
    @assertions += assertions.to_i
    @failures   += failures.to_i
    @errors     += errors.to_i
end

# output a usage message and exit
def output_usage
    puts "Usage: #{File.basename($0)} [OPTION] ..."
    puts "Run all project unit tests and aggregate their output\n\n"
    puts "-h, -?             Display this message"
    puts "-n                 Do not attempt to compile tests"
    puts "-d <directory>     Restrict test run to <directory>\n\n"
    
    exit
end

##################### Main Script #####################

# instance variables to hold statistics, etc.
@messages   = []
@tests      = 0
@failures   = 0
@errors     = 0
@assertions = 0
@rootdir    = File.dirname(__FILE__) + '/../..'
@compile    = true

# parse command line arguments
begin
    GetoptLong.new(
      [ '-?', '-h', GetoptLong::NO_ARGUMENT ],
      [ '-n', GetoptLong::NO_ARGUMENT ],
      [ '-d', GetoptLong::REQUIRED_ARGUMENT ]
    ).each do |opt, arg|
        case opt
        when '-?'
            output_usage
        when '-n'
            @compile = false
        when '-d'
            @rootdir = arg
        end
    end
rescue
    output_usage
end

# expand the root directory to an absolute path
@rootdir = File.expand_path(@rootdir)

# run a 'make test' on the root directory (if requested), outputting only stderr
errors = false
if @compile
    puts "Running a 'make test' on '#{@rootdir}'..."
    Open3.popen3("make -C #{@rootdir} test") do |stdin, stdout, stderr|
        while output = stderr.gets
            if output =~ /\*\*\*/
                errors = true
            end
            print "\n#{output.chomp}" 
        end
    end
    
    if errors
        puts "\n\n"
        exit(1)
    end
end

# run each collected test and capture the output
puts "Running tests in '#{@rootdir}'...\n\n"
collect_tests(@rootdir).each do |test|
    Open3.popen3("#{test} -c") do |stdin, stdout, stderr|
        line    = 0
        message = ''
        summary = ''
        while output = stdout.gets
            output.chomp!
            
            # if the line starts with a colon, it is a summary line
            if output =~ /^:/
                process_summary(output)
                line = -1
            
            # if this is a test info line (2nd line of a test file or line after a .)
            elsif line == 1
                summary = output
            
            # if the line is just '.' it is the end of a test message and start of a new test
            elsif output == '.'
                process_test(summary, message)
                summary = ''
                message = ''
                line = 0
                
            # in this case, we are in a message and this string needs to be added to the message
            else
                message += output + "\n"
                
            end 
            
            line += 1
        end
    end
end

# print "done! message"
puts " done!"

# go through all messages we have collected 
count = 1
@messages.each do |message|
    puts "\n#{count}) #{message}"
    count += 1
end

# change the console color
if @errors > 0
    puts "\x1b#{CONSOLE_COLOR_RED}"
elsif @failures > 0
    puts "\x1b#{CONSOLE_COLOR_YELLOW}"
else
    puts "\x1b#{CONSOLE_COLOR_GREEN}"
end
    
# print the final summary
puts "==========================================================================="
puts "#{@tests} tests, #{@assertions} assertions, #{@failures} failures, #{@errors} errors"

# reset the console color
puts "\x1b#{CONSOLE_RESET}"
