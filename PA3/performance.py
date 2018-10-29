#!/usr/bin/env python

from __future__ import division
import sys
import numpy as np
# Uncomment the following two lines to allow the script to run on a headless machine FIXME
# import matplotlib as mpl
# mpl.use("agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from matplotlib.ticker import LinearLocator, FormatStrFormatter
from timeit import timeit

T_CONVERSION=100

myinput = None

# Ensure we have a good input function to use on either python 2 or python 3
if sys.version_info < (3, 0):
    myinput = raw_input
else:
    myinput = input

# Fetches data from preformatted files
def get_data(fname):

    if not fname.endswith(".txt"):
        print("WARNING; expected a data file, but file name %s does not appear to end in .txt." % fname)
        myinput("If you're sure this is OK, press Enter to continue; otherwise, press Ctrl+C to terminate.")

    times = []
    res = []
    req = []

    # Open the file
    f = open(fname)
    # For each line
    for line in f.readlines():
        # Split the comment-separated data into an array
        d = line.split(",")
        if len(d) != 3 and line != "":  # try to deal with blank lines
            print("Error: File Not Formatted Properly")
            print("Expecting: #req threads, #res threads, time")
            exit()
        # Append the split data to a list for plotting
        if line != "":
            req.append(int(d[0]))
            res.append(int(d[1]))
            times.append(float(d[2]))

    return res, req, times

def generate_data(exe):
    req_list  = []
    res_list  = []
    time_list = []

    # Generate a range for each type of thread
    ### MODIFY HERE ###
    reps = 10
    for req in range(1, 10):
        for res in range(1, 10):
            # Declare parameters and the call arguments for the subprocess.call
            # There are two alternative approaches here; this first one feels semantically better
            # as it's equivalent to `./multi-lookup.o 5 5 serviced.txt results.txt names1.txt names2.txt names3.txt names4.txt names5.txt`
            # and passes each file as its own argument to be parsed

            # Change the below line if your names files are not in a directory called "input" or not called "names"
            # or choose whichever approach you like better to argument passing FIXME

            name_files = ["input/names%d.txt" % i for i in range(1, 6)]
            parameters = "%s %s" % (req, res)
            call_arguments = """["%s", "%s", "%s", "serviced.txt", "results.txt" """ % ("./"+str(exe), req, res)
            for name in name_files:
                call_arguments += ',"%s"' % name
            call_arguments += "]"

            # The legacy method of passing arguments, from the original provided script
            # This is equivalent to `./multi-lookup.o 5 5 results.txt serviced.txt "names1.txt names2.txt names3.txt names4.txt names5.txt"
            # NOTE: The final argument is quoted, so that entire quoted string will show up as argv[5] in your program!!!
            # The order of results.txt and serviced.txt is also interchanged from what the writeup indicates it probably should be!
            # Be careful of this before commenting this back in and using it, but FIXME maybe this is useful

            # name_files = "names1.txt names2.txt names3.txt names4.txt names5.txt"
            # parameters = "%s %s" % (req, res)
            # call_arguments = str("""["%s", "%s", "%s", "results.txt", "serviced.txt", "%s"]""" % ("./"+str(exe), req, res, name_files))

            # Time the program using timeit
            time = timeit(stmt = "subprocess.call(%s)" % call_arguments, setup = "import subprocess", number=reps) * T_CONVERSION / reps

            # Uncomment the next line to print how much time it took FIXME
            # print("Time: %s\n" % time)

            # Uncomment the next two lines for use as a quick-and-dirty method to removing outliers from the data set FIXME
            # Adjust the constants 20 and 12 according to your system! These were good for my code running on Elra; profile it on your machine first before using!!
            # if time > 20:
            #     time = 12

            # Store the data
            req_list.append(req)
            res_list.append(res)
            time_list.append(time)

    return req_list, res_list, time_list

# Takes the data input and plots it to a 3D graph
def plot(data):
    # Split the data into its components
    res, req, times = data

    # Format requestor and resolver data
    resset = set(res)
    reqset = set(req)
    req2d, res2d = np.meshgrid(sorted(resset), sorted(reqset))
    
    # Format time data
    Z = np.zeros(shape=(max(reqset)+1, max(resset)+1))
    for ind, (i, j) in enumerate(zip(req, res)):
        Z[i, j] = times[ind]
    Z = Z[min(reqset):, min(resset):]

    # Plot output
    f = plt.figure()
    ax2 = plt.axes(projection='3d')
    ax2.set_title('Time vs. Thread Count')
    # Remove the cstride and rstride arguments if the map doesn't look properly colored FIXME
    # This issue probably shouldn't happen, but it might happen with mock data.
    surf2 = ax2.plot_surface(req2d, res2d, Z, cmap=cm.coolwarm, cstride=1, rstride=1, linewidth=0)
    ax2.set_xlabel("# Requester Threads")
    ax2.set_ylabel('# Resolver Threads')
    ax2.set_zlabel('Time')
    f.colorbar(surf2, shrink=0.5, aspect=5)
    plt.savefig("performance.png")
    # Comment the next line out if you are working on a headless machine or the script will probably crash FIXME
    plt.show()

# An example of data that works with the 3d plot
def mock_data():
    x = []
    y = []
    z = []

    # Generate semi-arbitrary ranges and append data points
    for i in range(4, 19):
        for j in range(20, 30):
            x.append(i)
            y.append(j)
            z.append(np.random.rand())
    return x, y, z

def serialize_data(data):
    # Basic but untested data serialization method
    # Should conform to the specification of the `get_data` method
    req, res, times = data

    fname = myinput("Enter a file name for data serialization: ")
    with open(fname, "w") as f:
        for i in range(len(req)):
            f.write("%d,%d,%d\n" % req[i], res[i], times[i])


# When called interactively, treat this as main()
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: Missing Arguments")
        exit()
    elif len(sys.argv) > 2:
        print("Error: Extra Arguments")
        exit()

    # Input arguments
    exe = sys.argv[1]

    # Change the commenting of some of the following lines to test with mock data or open a saved file FIXME
    # Still assumes we only need to pass one argument, which is either the location of the runnable program
    # or the location of the data file, so only have one of these uncommented at once

    # data = get_data(exe)
    # data = mock_data()
    data = generate_data(exe)

    plot(data)

    # Uncomment the following line to save your data to a file for analysis or replotting FIXME
    # serialize_data(data)
