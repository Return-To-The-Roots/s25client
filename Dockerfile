RUN mkdir -p /app/bin /app/share /stats

COPY ./build/bin/launch_game /app/bin/
COPY ./build/share /app/share/
COPY ./build/share /app/share/

# Make the executable executable (if needed)
RUN chmod +x /app/bin/ai-battle

# Set the working directory
WORKDIR /app
ENV MAX_GF="120001" \
    STATS_PERIOD="500" \
    RUN_ID="000" \
    RUN_SET_ID="000"

# Set the entrypoint to run your executable
ENTRYPOINT ["/app/bin/launch_game", \
            "--maxGF", "${MAX_GF}", \
            "--stats_period", "${STATS_PERIOD}", \
            "-r", "${RUN_ID}", \
            "--run_set_id", "${RUN_SET_ID}"]

# Volume for stats (will be mounted at runtime)
VOLUME ["/stats"]
