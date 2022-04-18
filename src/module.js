// htmlparser2 does not fix malformed html. The website we have
// uses a lot of malformed html including no tbody or anything.
// However, it does have a performance benefit since we do not need to fixup
// the malformed html but would require more processing in the longterm.
// const htmlparser2 = require('htmlparser2');
const cheerio = require('cheerio');
const fs = require('fs');

const id_query_url = 'http://cydel.net/idlook.php';
const character_query_url = 'http://cydel.net/dicelook.php';

const character_selector =
  'body > center > table:nth-child(3) > tbody > tr:nth-child(2) > ' +
  'td:nth-child(6)';

const headers = {
  'Accept-Encoding': 'gzip, deflate',
  'Content-Type': 'application/x-www-form-urlencoded',
};

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

  const response = await fetch(url, {
    method: 'POST',
    headers: headers,
    body: new URLSearchParams(request_body),
  });
  const response_text = await response.text();
  return response_text;
}

function extractCharacterFromIDQuery(response) {
  const $ = cheerio.load(response);
  return {
    character_name: $(character_selector).html(),
    tag_in_name: $(character_selector).get(0).childNodes.length == 2,
  };
}

function createTSVFromCharacterQuery(response, tag_in_name) {
  const table_selector =
    'body > center > table:nth-child(' +
    (tag_in_name + 3) +
    ') > tbody > ' +
    'tr:not(:nth-child(1))';

  const $ = cheerio.load(response);
  let tsv = '';
  console.log($('body > center > table:nth-child(3) > tbody').html());
  $(table_selector)
    .children('td')
    .each((idx, elem) => {
      let builder;
      // This should let us skip row 0, col 0 from generating newline
      // Doing a switch case would complicate the issue a lot since we don't
      // want to fall through from col_no 7 to any other columns and
      // still want to append a tab character
      let col_no = idx - Math.floor(idx / 9) * 9;
      if (col_no === 6) {
        builder = $(elem).find('a').attr('href');
      } else {
        builder = $(elem).html();
      }

      // Short circuit and don't check builder string if not in user-controlled
      // text columns.
      if (col_no >= 5 && col_no <= 7 && builder.includes('"')) {
        builder = '"' + builder.replace(/"/g, '""') + '"';
      }

      if (col_no == 8) {
        builder += '\n';
      } else {
        builder += '\t';
      }

      tsv += builder;
    });

  return tsv;
}

async function crawl(id) {
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
  if (typeof id_query_results.character_name === 'undefined') {
    throw `Error: Roll with ID ${id} not found.`;
  }

  try {
    html_response = await queryRolls(
      'character',
      id_query_results.character_name
    );
  } catch (e) {
    console.error('Error: ', e);
    throw `Error when fetching rolls with character ${character}`;
  }

  return createTSVFromCharacterQuery(
    html_response,
    id_query_results.tag_in_name
  );
}

crawl(1).then((tsv) => {
  console.log(tsv);
});