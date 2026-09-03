[![progress-banner](https://backend.codecrafters.io/progress/shell/9dfa99f8-1eca-4cd1-a92b-40725fcc5ecc)](https://app.codecrafters.io/users/nscurvy?r=2qF)

This is just my run through "Build your own shell". I will be refactoring some of the code. I don't currently *plan* to
implement any more features. However, if I feel inspired, I might add on to it.

#### TODO
##### Code Correctness:
- Create more unit tests <!-- TODO: Create more unit tests -->
- Test with valgrind a few more times. 
- Lint and correct code to remove compiler/linter warnings. <!-- TODO: Create more unit tests -->
- Finish applying compiler Nonnull attributes to functions and arguments. <!-- TODO: Create more unit tests -->

##### Code aesthetics:
- Consistent naming scheme. 
- Consistent function names
- Rename some variables to be more descriptive.

##### Refactoring:
- Reorganize modules to have less responsibilities:
  - [ ] Split parser module:
    - [ ] Move the String List to its own module
    - [ ] Move tokenizing to its own module
    - [ ] Define an AST and move it to its own module
  - [ ] Refactor declare module: 
    - [ ] Rename to hash_table
    - [ ] Create variables module to handle shell variables
  - [ ] Clean the jobs module: 
    - [ ] Remove the static array and all functions that reference it.
    - [ ] Create better separation between job state and output
    - [ ] Create a more consistent way of reaping jobs
  - [ ] Split up the exec module:
    - [ ] Extract the repl function to its own module.
    - [ ] Focus exec module on command execution.
    - [ ] Remove unused redundant functions
    - [ ] Move the facilities for finding jobs on the PATH to its own module.
  - [ ] Reorganize expansions
  - [ ] Create a string builder/buffer to eliminate reliance on static buffers with arbitrary limits.
- Some functions could be merged so that they are more efficient. Some functions need splitting.
- I need to make sure that I have established a convention re: struct pointer ownership and that everything follows it.
- exec needs to be reorganized.

##### Security:
- There are sections where it is assumed that the user will input data which falls within certain bounds. I need to enforce and check those bounds.
  - Default argument length and default line length need to both be reasonable and enforced. 
- I need to audit the code for use after frees and off by ones. They don't happen within the parameters of codecrafters or my testing, but I need to test it more.

##### ***<u>Documentation for functions</p>***

### Note about AI: 
I used Claude to ask design/architecture questions, generate working examples of readline code, and
general help with bugfixes, such as figuring out where a bug might be coming from. This is my *preferred* use of AI.