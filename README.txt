We have a loop process that stops only if we choose it.Inside there is a call to the function that reads the input given by the user.Inside this function
after it reads it checks what type of instruction it is (simple,sequences,pipe,redirection) and also clears the input of blanks and whatever else we need to handle.Then we handle the
cases exit and cd.Then we get all cases for all types of commands by calling the corresponding function.Each function handles each case in the corresponding way
and I cover all cases, even the errors.
