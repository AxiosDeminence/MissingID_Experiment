// htmlparser2 does not fix malformed html. The website we have
// uses a lot of malformed html including no tbody or anything.
// However, it does have a performance benefit since we do not need to fixup
// the malformed html but would require more processing in the longterm.
// const htmlparser2 = require('htmlparser2');
const cheerio = require('cheerio');
const fs = require('fs');
const path = require('path');

/* How often to crawl (with a variance) and update in milliseconds*/
const crawl_interval = 5 * 60 * 1000;
const crawl_interval_variance = 45 * 1000;

// We need to use different URLs to query and forms depending on the data
// we use to query.
const id_query_url = 'http://cydel.net/idlook.php';
const character_query_url = 'http://cydel.net/dicelook.php';

// CSS Selectors to select the table and relevant attributes in the table
// Character and id selectors must be used in context of the table
const table_selector = 'body > center > table:nth-of-type(2) > tbody';
const character_selector = 'tr:nth-child(2) > td:nth-child(6)';
const rolls_selector = 'tr:not(:nth-child(1))';

// Headers used for the POST Query
const headers = {
  'Accept-Encoding': 'gzip, deflate',
  'Content-Type': 'application/x-www-form-urlencoded',
};

/**
 * Using a native addon results in 64 bytes from being definitely lost
 * and 304 bytes from being possibly lost.
 **/
const my_addon = require('../build/Release/addon.node');

/**
 * Queries the perma-roller for the roll(s) with the specified key attributes.
 * @param {string} type The type of query to be executed
 * @param {string} key  The key value of the query
 * @returns
 */
async function queryRolls(type, key) {
  let url, request_body;

  switch (type) {
    case 'id':
      url = id_query_url;
      request_body = {
        name: key,
        search: 'Search ID!',
      };
      break;
    case 'character':
      url = character_query_url;
      request_body = {
        name: key,
        'lets roll!': 'Search!',
      };
      break;
    default:
      throw TypeError(`Type ${type} not expected`);
  }

  // Create and send the response body which should be the malformed HTML
  const response = await fetch(url, {
    method: 'POST',
    headers: headers,
    body: new URLSearchParams(request_body),
  });
  const response_text = await response.text();
  return response_text;
}

/**
 * Extract the character from the query using the roll ID.
 * @param {string} response The HTML response of the query. May be malformed.
 * @returns {string} The name of the character who made the ID
 */
function extractCharacterFromIDQuery(response) {
  // Load the response using cheerio's parser5 (which is HTML-5 compliant
  // and will fix some malformed HTML)
  const $ = cheerio.load(response);

  // We use .html() here since some characters may have html in them: such as
  // the "Pacman ghosts".
  return $(table_selector).find(character_selector).html();
}

/**
 * Extract all rolls from a query using the character name and format them into
 * a DSV format.
 * @param {text} response The HTML response of the query. May be malformed.
 * @param {quote: string, delimiter: string} format_opts An object
 *     containing options for formatting the DSV
 * @param {Array<BigInt64>} running_ids A mutable array of the found ids
 * @returns {dsv} The rolls formatted in the DSV.
 */
function createDSVFromCharacterQuery(response, format_opts, running_ids) {
  // Selects the table to iterate the rows of
  const quote_token = format_opts['quote'];
  const delimiter_token = format_opts['delimiter'];
  const quote_regex = RegExp(quote_token, 'g');

  const $ = cheerio.load(response);

  let dsv = '';
  let col_no = 0;

  $(table_selector)
    .find(rolls_selector)
    .children('td')
    .each((_idx, elem) => {
      let builder;
      // This should let us skip row 0, col 0 from generating newline
      // Doing a switch case would complicate the issue a lot since we don't
      // want to fall through from col_no 7 to any other columns and
      // still want to append a tab character
      if (col_no === 6) {
        // Relative URLs will end up being ignored. This is fine for our use
        // case since the input should normally have been a full URL.
        builder = $(elem).find('a').attr('href');
      } else {
        builder = $(elem).html();
      }

      // Short circuit and don't check builder string if not in user-controlled
      // text columns.
      if (
        col_no >= 5 &&
        col_no <= 7 &&
        (builder.includes(quote_token) || builder.includes(delimiter_token))
      ) {
        builder =
          quote_token +
          builder.replace(quote_regex, quote_token + quote_token) +
          quote_token;
      }

      // If we are at the end of the row, we add in a newline, otherwise
      // add in the delimiter token
      switch (col_no) {
        case 8:
          builder += '\n';
          col_no = 0;
          break;
        case 0:
          running_ids.push(builder);
        default:
          builder += delimiter_token;
          ++col_no;
          break;
      }

      dsv += builder;
    });

  return dsv;
}

/**
 * Finds the character who owns the ID and then returns a DSV-formatted
 * string of all of the character's rolls.
 * @param {number} id The lowest missing ID to query for the character's rolls.
 * @param {quote: string, delimiter: string} format_opts An object containing
 *     format options for the DSV format.
 * @param {Array<BigInt64>} running_ids A mutable array of the found ids
 * @returns {character: string, dsv: string} Rolls formatted in a DSV
 */
async function crawl(id, format_opts, running_ids) {
  let html_response;
  try {
    html_response = await queryRolls('id', id);
  } catch (e) {
    console.error('Error: ', e);
    /* Done to propogate fetch errors */
    throw `Error when fetching roll with ID ${id}`;
  }

  const id_query_results = extractCharacterFromIDQuery(html_response);

  /* Character will be undefined if no roll with ID was found*/
  if (typeof id_query_results === 'undefined') {
    throw `Error: Roll with ID ${id} not found.`;
  }

  try {
    html_response = await queryRolls('character', id_query_results);
  } catch (e) {
    console.error('Error: ', e);
    throw `Error when fetching rolls with character ${character}`;
  }

  return {
    character: id_query_results,
    dsv: createDSVFromCharacterQuery(html_response, format_opts, running_ids),
  };
}

/**
 * Processes the command line arguments passed from the user
 * @param {Array<string>} argv The command line arguments from the user
 * @returns {Object} Map-like structure of the parameters
 */
function handleCLIArgs(argv) {
  const minimist = require('minimist');

  // List of settings and their defaults/aliases. Error on unknown CLI arg.
  /**
   * If --separate-character-files is specified, --output should be a directory.
   * Else, --output should be a singular file
   **/
  const minimist_settings = {
    string: ['input', 'quote', 'delimiter', 'base-dir', 'output'],
    boolean: ['separate-character-files'],
    alias: {
      i: 'input',
      o: 'output',
      q: 'quote',
      d: 'delimiter',
      dir: 'base-dir',
    },
    default: {
      output: 'rolls.tsv',
      quote: '"',
      delimiter: '\t',
      'separate-character-files': false,
      'base-dir': '.',
    },
    unknown: (param) => {
      throw `Unknown parameter ${param}`;
    },
  };

  try {
    const minimist_arguments = minimist(argv, minimist_settings);
    // Regex used to check if the quote or delimiter argument is a
    // single-character, ASCII valid field.
    const ascii_regex = new RegExp(/^[\x00-\x7F]$/);

    if (!ascii_regex.test(minimist_arguments['quote'])) {
      throw (
        'Expected single-character ASCII quote field, got: ' +
        minimist_arguments['quote'].toString()
      );
    } else if (!ascii_regex.test(minimist_arguments['delimiter'])) {
      throw (
        'Expected single-character ASCII delimiter field, got: ' +
        minimist_arguments['delimiter'].toString()
      );
    } else if (Array.isArray(minimist_arguments['output'])) {
      throw (
        'Expected single output destination, got: ' +
        minimist_arguments['output'].toString()
      );
    }

    const input = [];

    if (Array.isArray(minimist_arguments['input'])) {
      // We use this instead of .apply or spread operators because of the
      // potential for number of files to exceed JavaScript's callstacks
      minimist_arguments['input'].forEach((elem) => {
        input.push(elem);
      });
    } else if (typeof minimist_arguments['input'] !== 'undefined') {
      input.push(minimist_arguments['input']);
    }

    const input_files = [];

    input.forEach((elem) => {
      // TODO: Something to handle regular expressions or pattern substitution
      const matching_files = elem;
      input_files.push(
        path.resolve(minimist_arguments['base-dir'], matching_files)
      );
    });

    output = path.resolve(
      minimist_arguments['base-dir'],
      minimist_arguments['output']
    );

    // We return the flattened array of files

    return {
      input_files: input_files,
      output: output,
      quote: minimist_arguments['quote'],
      delimiter: minimist_arguments['delimiter'],
      output_type: minimist_arguments['separate-character-files']
        ? 'directory'
        : 'file',
    };
  } catch (e) {
    console.error(e);
    console.info('Refer to README.md for usage');
    return;
  }
}

function main() {
  // Handle arguments that are not the node version and source file
  const user_args = handleCLIArgs(process.argv.slice(2));
  if (typeof user_args === 'undefined') {
    return;
  }

  const running_ids = Array.from(
    my_addon.compileIDs(
      user_args['input_files'],
      user_args['quote'],
      user_args['delimiter']
    )
  );
  const format_opts = {
    quote: user_args['quote'],
    delimiter: user_args['delimiter'],
  };

  const value_headers = [
    'ID',
    'BD',
    'CD',
    'LD',
    'MD',
    `${format_opts['quote']}Character${format_opts['quote']}`,
    `${format_opts['quote']}URL${format_opts['quote']}`,
    `${format_opts['quote']}Purpose${format_opts['quote']}`,
    'Time',
  ];

  const dsv_header = value_headers.join(format_opts['delimiter']) + '\n';

  // We use nested timeouts in order to ensure that there are at least
  // crawl_interval milliseconds
  let previous_missing_id = -1;
  let iteration = 0;
  let workflow = setTimeout(async function work() {
    try {
      const missing_id = my_addon
        .missingID(BigInt64Array.from(running_ids))
        .toString();
      if (missing_id === previous_missing_id) {
        throw 'Same ID is still missing';
      }
      const crawl_results = await crawl(missing_id, format_opts, running_ids);
      const output_file =
        user_args['output_type'] === 'directory'
          ? path.resolve(user_args['output'], crawl_results['character'])
          : user_args['output'];
      previous_missing_id = missing_id;
      // If the file does not exist, then we want to append headers
      fs.promises
        .access(output_file, fs.constants.F_OK)
        .catch((err) => {
          fs.appendFile(output_file, dsv_header, (err) => {
            if (err) throw err;
          });
        })
        .finally(() => {
          fs.appendFile(output_file, crawl_results['dsv'], (err) => {
            if (err) throw err;
          });
        });
      workflow = setTimeout(work, crawl_interval);
      console.log(`iteration: ${++iteration} complete`);
    } catch (e) {
      // Stops the nested timeout
      clearTimeout(workflow);
      throw e;
    }
  }, 0);
}

// ids.forEach((elem) => {
//   console.log(elem);
// });

main();
