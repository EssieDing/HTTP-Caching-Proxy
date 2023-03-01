FROM gcc:9

RUN mkdir /var/log/erss
ADD . /var/log/erss
WORKDIR /var/log/erss

RUN chmod +x run.sh

ENTRYPOINT ["./run.sh"]