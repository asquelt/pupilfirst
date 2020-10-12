FROM ruby:2.7.1-slim

ENV APPROOT /usr/src/app
ENV NODE_MAJOR 12
ENV PATH /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/bin:/opt/yarn/bin

RUN apt-get update && \
    apt-get -y install imagemagick libpq-dev autoconf libtool gnupg curl git xz-utils libxml2 build-essential patch zlib1g-dev liblzma-dev && \
    rm -rf /var/cache/apt /usr/share/doc

# throw errors if Gemfile has been modified since Gemfile.lock
RUN bundle config --global frozen 1

WORKDIR $APPROOT
RUN useradd -d $APPROOT -s /bin/false app && chown -R app:app $APPROOT

COPY containers/js_setup.sh /usr/bin/
RUN bash /usr/bin/js_setup.sh

COPY Gemfile Gemfile.lock ./
RUN bundle install

#COPY --chown=app:app . .
COPY . .
RUN chown app:app -R $APPROOT && chmod 777 $APPROOT/tmp $APPROOT/log $APPROOT/storage

RUN yarn install --network-timeout 1000000

EXPOSE 3000

COPY containers/entrypoint.sh /usr/bin/
RUN chmod +x /usr/bin/entrypoint.sh

USER app

VOLUME $APPROOT/storage

ENTRYPOINT ["/usr/bin/entrypoint.sh"]
CMD ["bundle", "exec", "rails", "server", "-b", "0.0.0.0"]
