#!/bin/sh
OUTPUT=$1; shift
SCRIPT="$*"

echo "#!/bin/sh" > "$OUTPUT"
echo "$SCRIPT \$*" >> "$OUTPUT"

chmod +x "$OUTPUT"
