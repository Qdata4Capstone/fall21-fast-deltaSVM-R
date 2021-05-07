convertFromGKM <- function(pos_file, neg_file, out_file) {

  outfile <- file(out_file, "wt")

  for (filepath in list(pos_file, neg_file)) {
    data = read.table(file=filepath)
    if (filepath == pos_file) {
        header <- ">1"
    }
    else {
        header <- ">0"
    }

    for (i in 1:nrow(data)) {
      if (i %% 2 == 1 && substr(data[i, ], 0, 1) != ">") {
        error_msg <- paste("Error: Expected '>' on line", i, "please check the file is in fasta format")
        close(outfile)
        stop(error_msg)
      }
      if (i %% 2 == 1) {
        write(header, outfile, append=TRUE)
      }
      if (i %% 2 == 0) {
        write(data[i,], outfile, append=TRUE)
      }
    }
  }

  close(outfile)
}
