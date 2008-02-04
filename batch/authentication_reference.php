<?php 

////////////////////////////////////////////////////
//
// Authentication server, reference implementation 0.1
// 
// To run authority X, this script needs to be 
// adapted and be callable from a web browser as 
// http://X/armaauth/0.1
//
// the easiest way to do so is to name the script index.php
// and place it into an appropriate armaauth/0.1 
// directory on a web server that supports php.
//
////////////////////////////////////////////////////

// however, before they are used, %u is replaced by the username.

function substitutions( $fix, $user )
{
    return str_replace( "%u", $user, $fix );
}


// this function reports the result of the check back to the
// qerying AA server.

function conclude($statusCode, $msg)
{ 
    // if you do want to abuse return error codes (the servers
    // work fine with parsing the text response), uncomment
    // the next line. The request is technically always successful.
    $statusCode = 200;

    // the status codes passed in have been chosen somewhat
    // meaningfully (200: ok, 404: something not found,
    // 401: password failure) but still violate the standard.

    // report error code in header
    header("Status: $statusCode", true, $statusCode); 
    header("Content-Type: text/plain");

    // print message
    die("$msg\n"); 
} 

// read authority from gloval variables
$authority = $_SERVER['HTTP_HOST'];

////////////////////////////////////////////////////
// Bits you need to change follow.
////////////////////////////////////////////////////

// You should definitely check that the hostname the game
// server used to contact you is the one you intend it
// to use; otherwise, there may be problems with web
// servers known under different names. You should uncomment
// this and add your real authority hostname.

/*
if ( $authority != "authority" )
    conclude( 404, "WRONG_HOST" );
*/

// The user "database": fetch the password for a user.
// If you are fine with storing plaintext passwords,
// adapt this function; otherwise, adapt getPasswordHash().

function getPassword( $user )
{
    // the user/password database. For very small groups
    // of users, you may just get away with expanding this
    // array.
    $passwords= array (
        'test' => 'password' // clever choice there, test
        );

    $password = $passwords[ $user ];
    if ( NULL == $password )
        return NULL;

    // return a pair of username and password.
    // it is important that you return the username
    // exactly as it appeared in the database.
    // If the username lookup is case insensitive,
    // the rest of the script and the game servers
    // need to know what the correct form of the name 
    // is.
    return array( $user, $password );
}

// these two functions return prefix and suffix for the md5 
// hash method. They are prepended/appended to the password
// before md5 is applied on it. Adapt them to the way your
// md5 password hash is stored in your database.

// IMPORTANT: if you want to keep the %u (a good idea for
// security, prevents precomputation attacks on the passwords)
// user name lookup needs to be case sensitive, or there will
// be unexplainable password failures. 

function getPrefix()
{
    return "%u:aaauth:";
}

function getSuffix()
{
    global $authority;
    return ":$authority";
}

// You do not need plain text passwords. The
// checks on the passwords are done to a hash function
// thereof. You can just as well precompute the
// work of this function and store the result,
// and maybe your user database already contains
// the one or other hash. phpBB, for example,
// stores md5( $password ).

function getPasswordHash( $user, $method )
{
    // fetch the plaintext password
    $userInfo = getPassword( $user );
    if ( NULL == $userInfo )
        return NULL;

    // unpack the data
    $trueUser = $userInfo[0];
    $password = $userInfo[1];

    // check that neither prefix nor suffix conain %u if $trueUser != $user
    if ( $trueUser != $user && ( strpos( getPrefix(), '%u' ) !== FALSE || strpos( getPrefix(), '%u' ) !== FALSE ) )
    {
        conclude(404, 'UNKNOWN_USER ' . $user . ' Do not use %u in pre/suffix if your user database is making case-insensitive lookups.');
    }

    // two methods are currently supported
    // by server and client, bmd5 (broken md5)
    // and md5. Both use the md5 hash algorithm.

    switch( $method )
    {
    case 'bmd5':
        // bmd5 quirk: the hash is computed for password + a trailing 0
        $password = "$password" . chr(0); 
        break;

    case "md5":
        // md5 adds the prefix and suffix before calculating
        // the hash. The example values are chosen so that the
        // resulting hash is the same one you find in .htdigest
        // files and that is used in the digest http authentication
        // method. If you set the prefix and suffix to empty
        // strings, the resulting hash will be the one found
        // in phpBB user databases.
        $password =  substitutions( getPrefix(), $trueUser ) . "$password" . substitutions( getSuffix(), $trueUser );
        break;
    }

    // after that, both methods just calculate the md5 hash
    // and return that.
    return array( $trueUser, md5( $password ) );
}

// comma separated lists of methods you support. If, for example,
// you cannot generate the hash for the bmd5 method, just
// remove it from this list.

function getMethods()
{
    return "md5, bmd5";
}

////////////////////////////////////////////////////
// The rest of the script should not be changed, 
////////////////////////////////////////////////////

// Of course, the AA password query passes in some variables.
// The first is the query type; it can either be 
// 'methods' for a list of supported methods,
// 'params'  for a list of method parameters, or
// 'check'   to check a user's identity.

$query = @strtolower($_REQUEST['query'] . '');

switch ( $query )
{
case "methods":
    // give lists of supported methods. It's supposed to be "methods", 
    // followed by a comma and/or whitespace separated list of the methods.
    conclude(200, 'METHODS '. getMethods() );
    break;

// the other two queries pass in more parameters, both have
// the 'method' parameter that tells you which method should
// be used. Well behaved servers will pick one of the methods 
// you support.

case "params":
    $method = @strtolower($_REQUEST['method'] . ''); 

    // give method parameter info
    if ( $method == 'md5' )
    {
        // give prefix and suffix
        conclude( 200, "PREFIX " . getPrefix() . "\nSUFFIX " . getSuffix() );
    }
    
    if ( $method == 'bmd5' )
    {
        // bmd5 does not support or need extra parameters.
        conclude(200 , '' );
    }

    // we know nothing about this.
    conclude(404 , 'UNKNOWN_METHOD' );
    break;

case "check":
    $method = @strtolower($_REQUEST['method'] . ''); 

    // the last query passes in the username to check,
    $user = @$_REQUEST['user'] . ''; 

    // a salt value (128 bit hexcoded)
    $salt = @$_REQUEST['salt'] . ''; 

    // and a hash value the AA client has computed.
    $hash = @$_REQUEST['hash'] . ''; 

    // we're supposed to check whether that hash is correct;
    // to do so, we need to do the same operations to the
    // password the client has done.
    
    // first, the client computed a hash of the password 
    // with method-specific rules. <TV cook mode> we have
    // already prepared that here. </TV cook mode>.
    $userInfo = getPasswordHash( $user, $method );

    // check if user exists in the first place.
    if ( $userInfo == NULL )
    {
        conclude(404, 'UNKNOWN_USER ' . $user );
    }

    // unpack user info
    $trueUser            = $userInfo[0];
    $correctPasswordHash = $userInfo[1];

    // the operations the AA client did were not on the hex-encoded
    // hashes we have so far, but on binary packed variants thereof:
    $packedSalt                = pack("H*", $salt); 
    $correctPackedPasswordHash = pack('H*', $correctPasswordHash); 

    // the client then simply calculated the MD5 sum of the two
    // concatenated packed values.
    $correctHash = md5($correctPackedPasswordHash . $packedSalt); 

    // well, let's see if the client got it right!
    if (strcasecmp($hash, $correctHash) === 0)
    {
        // he did! Return OK, followed by the user's full name.
        conclude(200, 'PASSWORD_OK ' . $trueUser . '@' . $authority ); 
    }

    // he didn't.
    conclude(401, 'PASSWORD_FAIL' );

    // That's it. Note that any response other that a text beginning
    // with "PASSWORD_OK" and a 200 return code will be interpreted
    // by the server as an error and authentication will fail.

    // String comparisons for the control words (METHODS, PASSWORD_OK)
    // are case-insensitive on the server. We just like to return them
    // as caps becasue that looks more like CONTOL_CODES.
    break;

default:
    // we don't know what the server wants from us
    // if execution ends up here.
    conclude(404, 'UNKNOWN_QUERY');
}

?>
