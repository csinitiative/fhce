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

require 'rubygems'
require 'builder'

# Cucumber formatter to produce JUnit Report (XML) output
class ReportFormatter
    # create the XML builder and print out the <?xml ?> "tag"
    def initialize(io, step_mother, options={})
        @x = Builder::XmlMarkup.new(:target => $stdout, :indent => 4)
        @output_dir = File.join(FileUtils.pwd(), ENV['CUKE_OUTPUT_DIR']) if ENV['CUKE_OUTPUT_DIR']
    end

    # when a feature starts executing, dump the last feature (if this is not the first
    # feature to run) and do some setup of instance variables
    def feature_executing(feature)
        # dump the last feature (if this is not the first)
        dump_feature if @name

        # figure out the project-relative path to the file and convert it into a test name
        absolute_root = File.expand_path(ROOT_DIR) + '/'
        @name = File.expand_path(feature.file).sub(absolute_root, '')
        @name.gsub!(/\//, '_')
        @name.sub!(/\.func$/, '')

        # if we have an @output_dir, open a file for the current feature and point the @x
        # XML builder at the correct file
        if @output_dir
            filename = @name + '.xml'
            @file    = File.open(File.join(@output_dir, filename), "w+")
            @x       = Builder::XmlMarkup.new(:target => @file, :indent => 4)
        end

        # initialize counters, timers, etc.
        @tests     = 0
        @failures  = 0
        @start     = Time.now.to_f
        @scenarios = []

        # print the XML header
        @x.instruct!
    end

    # if a step fails, record the details of the failure
    def step_failed(step, regexp, args)
        @failure   = true
        @failtype  = step.error.inspect
        @failmsg   = step.error.message
        @failtrace = step.error.backtrace
    end

    # when a new scenario starts, record the details
    def scenario_executing(scenario)
        @scenario_start = Time.now.to_f
        @failure        = false
    end

    # when a scenario completes, record the details
    def scenario_executed(scenario)
        # increment tests and failures (if the scenario failed)
        @failures  += 1 if @failure
        @tests     += 1

        # add another scenario to the scenarios array
        scenario = {
            :name    => scenario.name,
            :time    => "%.3f" % (Time.now.to_f - @scenario_start),
            :failure => @failure
        }
        if @failure
            scenario[:failtype]  = @failtype
            scenario[:failmsg]   = @failmsg
            scenario[:failtrace] = @failtrace
        end
        @scenarios << scenario
    end

    # when we are all done running, dump the final feature
    def dump
        dump_feature
    end

    private

    # return a hash of feature
    def feature_attrs
        {
            :name     => 'functional.' + @name,
            :failures => @failures,
            :tests    => @tests,
            :errors   => 0,
            :time     => "%.3f" % (Time.now.to_f - @start)
        }
    end

    # this method will be called when we are done processing a feature...it will dump that
    # feature in JUnit report-style XML
    def dump_feature
        # output a complete <testsuite> in XML
        @x.testsuite(feature_attrs) do
            @scenarios.each do |scenario|
                unless scenario[:failure]
                    @x.testcase(:name => scenario[:name], :time => scenario[:time])
                else
                    @x.testcase(:name => scenario[:name], :time => scenario[:time]) do
                        @x.failure(:type => scenario[:failtype], :message => scenario[:failmsg]) do
                            scenario[:failtrace].each do |line|
                                @x << line.to_xs << "<br />\n"
                            end
                        end
                    end
                end
            end
        end

        # close the output file, if there is an output file open
        @file.close if @file
    end
end
