# 😲 What The Heck?

### 📂 DATA 

    This contains all the graphs with K vertices. (graph-{K})
    All derived from: https://zenodo.org/record/4010122
    Vertex-transitive Graphs On Fewer Than 48 Vertices - Royle, Gordon; Holt, Derek

### 📂 critical

    These are all the critical Vertex-transitive graphs

### 📄 unzip.py

    Neccessary for unzipping .gz files 
	⚠️ Should only be run on Ubuntu or linux devices. 
	It unzips the .tar and .gz files in /DATA

### 📄 index.py

	After running unzipy.py, then run index.py (using sage).
    Here this file checks all the DATA graphs if they are critical. 
    If so they are saved to "/critical". 

### 📄 multiprocess-index.py

	This runs the same function as 📃index.py with the added bonus of being faster than index.py 