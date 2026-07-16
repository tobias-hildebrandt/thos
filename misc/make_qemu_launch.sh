#!/bin/sh
OUTPUT=$1; shift
SCRIPT="$*"

cat > "$OUTPUT" << EOF
#!/bin/sh

# run and trace command to stderr
run() { echo \$* 1>&2; \$*; }

# run command
run $SCRIPT \$*

EOF

chmod +x "$OUTPUT"
