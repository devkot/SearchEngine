# SearchEngine

A tiny search engine featuring a trie as a primary data structure to improve performance. There is a custom HTTP server that supports GET requests and serves pages inside a specific directory. The directory contents are created by a bash script which splits text files into random HTML pages and adds links to other pages for indexing purposes. The Crawler is responsible for downloading the pages from the web server, analyzing them, and following the links to the rest pages or "web sites". After the crawling has finished, the Crawler supports remote commands including search, through a telnet connection. 

## Installation

Simply fork the project and save it into a directory giving it the required permissions with chmod 755 <directory name>
  
## Usage

While in the main folder in the project directory, run the makefile by typing ``` make ```

### Creating the web sites ``` ./webcreator.sh root_dir text_file w p ```
    root_dir: needs to be created before hand and will store the web sites
    text_file: text source to create the content of the web pages
    w: number of web sites to be created
    p: number of pages to be created per web site
    
### Starting the server 
``` ./myhttpd -p serving_port -c command_port -t number_of_threads -d root_dir ```

    Serving and command port: the ports the server will be listening on (eg. 8080)
    Number_of_threads: the number of threads the server will execute in parallel
    root_dir: the directory specified in the web creator
    
The server receives the following commands via telnet:
    ``` STATS, SHUTDOWN ```
    
### Crawler
``` ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL```

    host_or_IP: the domain name or the IP of the machine the server is running on
    port: the server's port
    command_port: the port on which to connect via telnet and query the crawler
    num_of_threads: same as above
    save_dir: the directory where the crawler will store the downloaded pages
    starting_URL: the URL of the web page where the crawling will start (eg. http://localhost:8080/siteX/pageY_Z.html) 
    
After the crawling has finished, you can connect to the command port via telnet and use the following commands:
    ```STATS, SEARCH word1 word2 ... word10, SHUTDOWN ```
    
## Authors
    Stylianos Kotanidis - University of Athens
    
## License 

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
